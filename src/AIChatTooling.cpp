#include "AIChatTooling.h"
#include "AIChatToolingInternal.h"
#include "AIService.h"
#include "ConfigManager.h"
#include "Global.h"
#include "Logger.h"
#include "PowerShellToolRunner.h"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <condition_variable>
#include <format>
#include <mutex>
#include <string>

#include "..\\thirdparty\\json.hpp"

namespace {

// 当前进程 PowerShell 全允许标志，用户选择"全允许"后本次进程内跳过后续确认。
std::atomic<bool> g_psAllowAllForProcess{false};

std::string TrimAsciiCopy(const std::string& text)
{
	size_t begin = 0;
	size_t end = text.size();
	while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
		++begin;
	}
	while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
		--end;
	}
	return text.substr(begin, end - begin);
}

std::string ToLowerAsciiCopyLocal(std::string text)
{
	for (char& ch : text) {
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return text;
}

std::string LocalFromWide(const wchar_t* text)
{
	if (text == nullptr || *text == L'\0') {
		return std::string();
	}
	const int size = WideCharToMultiByte(CP_ACP, 0, text, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 0) {
		return std::string();
	}
	std::string out(static_cast<size_t>(size), '\0');
	if (WideCharToMultiByte(CP_ACP, 0, text, -1, out.data(), size, nullptr, nullptr) <= 0) {
		return std::string();
	}
	if (!out.empty() && out.back() == '\0') {
		out.pop_back();
	}
	return out;
}

bool IsValidUtf8Text(const std::string& text)
{
	if (text.empty()) {
		return true;
	}
	return MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		text.data(),
		static_cast<int>(text.size()),
		nullptr,
		0) > 0;
}

std::string ConvertCodePage(const std::string& text, UINT fromCodePage, UINT toCodePage, DWORD fromFlags = 0)
{
	if (text.empty()) {
		return std::string();
	}

	const int wideLen = MultiByteToWideChar(
		fromCodePage,
		fromFlags,
		text.data(),
		static_cast<int>(text.size()),
		nullptr,
		0);
	if (wideLen <= 0) {
		return text;
	}

	std::wstring wide(static_cast<size_t>(wideLen), L'\0');
	if (MultiByteToWideChar(
		fromCodePage,
		fromFlags,
		text.data(),
		static_cast<int>(text.size()),
		wide.data(),
		wideLen) <= 0) {
		return text;
	}

	const int outLen = WideCharToMultiByte(
		toCodePage,
		0,
		wide.data(),
		wideLen,
		nullptr,
		0,
		nullptr,
		nullptr);
	if (outLen <= 0) {
		return text;
	}

	std::string out(static_cast<size_t>(outLen), '\0');
	if (WideCharToMultiByte(
		toCodePage,
		0,
		wide.data(),
		wideLen,
		out.data(),
		outLen,
		nullptr,
		nullptr) <= 0) {
		return text;
	}
	return out;
}

std::string LocalToUtf8Text(const std::string& text)
{
	if (text.empty()) {
		return std::string();
	}
	if (IsValidUtf8Text(text)) {
		return text;
	}
	return ConvertCodePage(text, CP_ACP, CP_UTF8, 0);
}

std::string Utf8ToLocalText(const std::string& text)
{
	if (text.empty()) {
		return std::string();
	}
	if (!IsValidUtf8Text(text)) {
		return text;
	}
	return ConvertCodePage(text, CP_UTF8, CP_ACP, MB_ERR_INVALID_CHARS);
}

void NormalizeJsonStringsToUtf8(nlohmann::json& value)
{
	if (value.is_string()) {
		value = LocalToUtf8Text(value.get<std::string>());
		return;
	}
	if (value.is_array()) {
		for (auto& item : value) {
			NormalizeJsonStringsToUtf8(item);
		}
		return;
	}
	if (value.is_object()) {
		for (auto& item : value.items()) {
			NormalizeJsonStringsToUtf8(item.value());
		}
	}
}

std::string JsonToLocalText(nlohmann::json value)
{
	NormalizeJsonStringsToUtf8(value);
	return Utf8ToLocalText(value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
}

void ReplaceAllInPlace(std::string& text, const std::string& from, const std::string& to)
{
	if (from.empty()) {
		return;
	}

	size_t pos = 0;
	while ((pos = text.find(from, pos)) != std::string::npos) {
		text.replace(pos, from.size(), to);
		pos += to.size();
	}
}

std::string SanitizeSingleLineText(std::string text)
{
	ReplaceAllInPlace(text, "\\r\\n", " ");
	ReplaceAllInPlace(text, "\\n", " ");
	ReplaceAllInPlace(text, "\\r", " ");
	ReplaceAllInPlace(text, "\\t", " ");
	ReplaceAllInPlace(text, "\r\n", " ");
	ReplaceAllInPlace(text, "\n", " ");
	ReplaceAllInPlace(text, "\r", " ");
	ReplaceAllInPlace(text, "\t", " ");

	std::string collapsed;
	collapsed.reserve(text.size());
	bool previousWhitespace = false;
	for (unsigned char ch : text) {
		if (std::isspace(ch) != 0) {
			if (!previousWhitespace) {
				collapsed.push_back(' ');
				previousWhitespace = true;
			}
			continue;
		}
		collapsed.push_back(static_cast<char>(ch));
		previousWhitespace = false;
	}
	return TrimAsciiCopy(collapsed);
}

bool TryDecodeTextToWide(const std::string& text, std::wstring& outWide)
{
	outWide.clear();
	if (text.empty()) {
		return true;
	}

	int wideLen = MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		text.data(),
		static_cast<int>(text.size()),
		nullptr,
		0);
	UINT codePage = CP_UTF8;
	DWORD flags = MB_ERR_INVALID_CHARS;
	if (wideLen <= 0) {
		wideLen = MultiByteToWideChar(
			CP_ACP,
			0,
			text.data(),
			static_cast<int>(text.size()),
			nullptr,
			0);
		codePage = CP_ACP;
		flags = 0;
		if (wideLen <= 0) {
			return false;
		}
	}

	outWide.assign(static_cast<size_t>(wideLen), L'\0');
	if (MultiByteToWideChar(
		codePage,
		flags,
		text.data(),
		static_cast<int>(text.size()),
		outWide.data(),
		wideLen) <= 0) {
		outWide.clear();
		return false;
	}
	return true;
}

std::string WideToUtf8(const std::wstring& text)
{
	if (text.empty()) {
		return std::string();
	}

	const int utf8Len = WideCharToMultiByte(
		CP_UTF8,
		0,
		text.data(),
		static_cast<int>(text.size()),
		nullptr,
		0,
		nullptr,
		nullptr);
	if (utf8Len <= 0) {
		return std::string();
	}

	std::string utf8(static_cast<size_t>(utf8Len), '\0');
	if (WideCharToMultiByte(
		CP_UTF8,
		0,
		text.data(),
		static_cast<int>(text.size()),
		utf8.data(),
		utf8Len,
		nullptr,
		nullptr) <= 0) {
		return std::string();
	}
	return utf8;
}

std::string ConvertUtf8ToGbkText(const std::string& text)
{
	if (text.empty()) {
		return std::string();
	}
	if (!IsValidUtf8Text(text)) {
		return text;
	}
	return ConvertCodePage(text, CP_UTF8, 936, MB_ERR_INVALID_CHARS);
}

std::string TruncateToolLogText(const std::string& text, size_t maxChars = 180)
{
	std::wstring wide;
	if (TryDecodeTextToWide(text, wide)) {
		if (wide.size() <= maxChars) {
			return text;
		}
		const std::string truncated = WideToUtf8(wide.substr(0, maxChars));
		if (!truncated.empty()) {
			return truncated + "...";
		}
	}

	if (text.size() <= maxChars) {
		return text;
	}
	return text.substr(0, maxChars) + "...";
}

std::string FormatToolLogText(const std::string& text)
{
	return TruncateToolLogText(SanitizeSingleLineText(text), 180);
}

std::string FormatToolLogJsonString(const std::string& jsonText)
{
	const std::string trimmed = TrimAsciiCopy(jsonText);
	if (trimmed.empty()) {
		return "null";
	}

	try {
		const nlohmann::json value = nlohmann::json::parse(trimmed);
		if (value.is_null() || (value.is_object() && value.empty())) {
			return "null";
		}
		return FormatToolLogText(value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
	}
	catch (...) {
		return FormatToolLogText(jsonText);
	}
}

// 构建用于文件日志的完整工具 JSON 字符串，不移除换行，便于排查工具原始输入/输出。
std::string FormatToolLogJsonStringFull(const std::string& jsonText)
{
	const std::string trimmed = TrimAsciiCopy(jsonText);
	if (trimmed.empty()) {
		return "null";
	}
	try {
		const nlohmann::json value = nlohmann::json::parse(trimmed);
		if (value.is_null() || (value.is_object() && value.empty())) {
			return "null";
		}
		return value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	}
	catch (...) {
		return jsonText;
	}
}

void LogInternalToolCallLine(const std::string& message)
{
	Logger::Instance().WriteAndIde("Tool", message);
}

void LogInternalToolRequest(const std::string& toolName, const std::string& argumentsJson)
{
	const std::string ideLine = ">> " + toolName + "(" + FormatToolLogJsonString(argumentsJson) + ")";
	const std::string fileLine = ">> " + toolName + "(" + FormatToolLogJsonStringFull(argumentsJson) + ")";
	Logger::Instance().WriteSplit("Tool", fileLine, ideLine);
}

void LogInternalToolResponse(const std::string& toolName, const std::string& resultJsonLocal, double elapsedMs)
{
	const std::string resultJsonUtf8 = LocalToUtf8Text(resultJsonLocal);
	const std::string ideLine = std::format(
		"<< {} ({:.1f}ms) {}",
		toolName.empty() ? "unknown_tool" : toolName,
		elapsedMs,
		FormatToolLogJsonString(resultJsonUtf8));
	const std::string fileLine = std::format(
		"<< {} ({:.1f}ms) {}",
		toolName.empty() ? "unknown_tool" : toolName,
		elapsedMs,
		FormatToolLogJsonStringFull(resultJsonUtf8));
	Logger::Instance().WriteSplit("Tool", fileLine, ideLine);
}

bool RequestToolExecutionFromMainThread(
	const std::string& toolName,
	const std::string& argumentsJson,
	std::string& outResultJson,
	bool& outOk,
	bool publicToolCall)
{
	outResultJson.clear();
	outOk = false;
	const HWND mainWindow = GetAIChatMainWindowForTooling();
	const UINT toolExecMessage = GetAIChatToolExecMessageForTooling();
	if (mainWindow == nullptr || !IsWindow(mainWindow) || toolExecMessage == 0) {
		return false;
	}

	ToolExecutionRequest request;
	request.toolName = toolName;
	request.argumentsJson = argumentsJson;
	request.publicToolCall = publicToolCall;

	const auto dispatchStart = std::chrono::steady_clock::now();
	const DWORD mainThreadId = GetWindowThreadProcessId(mainWindow, nullptr);
	if (mainThreadId != 0 && mainThreadId == GetCurrentThreadId()) {
		bool ok = false;
		const std::string resultJson = ExecuteToolCallOnMainThread(request.toolName, request.argumentsJson, ok);
		{
			std::lock_guard<std::mutex> lock(request.mutex);
			request.resultJson = resultJson;
			request.ok = ok;
			request.done = true;
		}
	}
	else {
		// 工具调用必须在主线程操作 IDE，但不能使用 PostMessage 排队，否则繁忙 UI 消息队列会让本地编辑空等几十秒。
		SendMessage(mainWindow, toolExecMessage, 0, reinterpret_cast<LPARAM>(&request));
	}

	{
		std::unique_lock<std::mutex> lock(request.mutex);
		request.cv.wait(lock, [&request]() {
			return request.done;
		});
	}

	const double dispatchMs = std::chrono::duration<double, std::milli>(
		std::chrono::steady_clock::now() - dispatchStart).count();
	if (dispatchMs >= 1000.0) {
		LogInternalToolCallLine(std::format(
			"main_thread_dispatch_slow tool={} elapsed_ms={:.1f}",
			toolName.empty() ? "unknown_tool" : toolName,
			dispatchMs));
	}

	outResultJson = request.resultJson;
	outOk = request.ok;
	return true;
}

std::string ExecuteToolCallImpl(
	const std::string& toolName,
	const std::string& argumentsJson,
	bool& outOk,
	bool publicToolCall,
	const std::function<bool()>& cancelCallback)
{
	outOk = false;

	if (toolName == "run_powershell_command") {
		nlohmann::json r;
		r["ok"] = false;
		r["disabled"] = true;
		r["error"] = "run_powershell_command disabled in silent MCP build to avoid IDE-hosted process instability";
		r["hint"] = "Use the external Codex shell for local PowerShell tasks; AutoLinker MCP source read/write tools are unchanged.";
		return JsonToLocalText(r);
	}

	if (toolName == "search_web_tavily" ||
		toolName == "fetch_url" ||
		toolName == "extract_web_document") {
		nlohmann::json r;
		r["ok"] = false;
		r["disabled"] = true;
		r["error"] = "web fetch/search tools disabled in silent MCP build";
		r["hint"] = "AutoLinker MCP keeps IDE source read/write tools only; use the external Codex environment for web lookups.";
		return JsonToLocalText(r);
	}

	if (toolName == "read_file" ||
		toolName == "read_files" ||
		toolName == "read_real_file" ||
		toolName == "search_code" ||
		toolName == "list_files" ||
		toolName == "refresh_workspace_mirror" ||
		toolName == "get_current_page_info" ||
		toolName == "get_current_eide_info" ||
		toolName == "refresh_dependency_catalog" ||
		toolName == "search_available_modules" ||
		toolName == "search_available_support_libraries" ||
		toolName == "edit_file" ||
		toolName == "multi_edit_file" ||
		toolName == "write_file" ||
		toolName == "diff_file" ||
		toolName == "restore_file_snapshot" ||
		toolName == "list_imported_modules" ||
		toolName == "add_module_to_project" ||
		toolName == "remove_module_from_project" ||
		toolName == "add_support_library_to_project" ||
		toolName == "compile_with_output_path") {
		std::string resultJson;
		if (!RequestToolExecutionFromMainThread(toolName, argumentsJson, resultJson, outOk, publicToolCall)) {
			return R"({"ok":false,"error":"main thread tool execution failed"})";
		}
		return resultJson;
	}

	nlohmann::json r;
	r["ok"] = false;
	r["error"] = "unknown tool: " + toolName;
	return JsonToLocalText(r);
}

} // namespace

std::string ExecuteToolCall(
	const std::string& toolName,
	const std::string& argumentsJson,
	bool& outOk,
	bool enableLog,
	const std::function<bool()>& cancelCallback,
	bool publicToolCall)
{
	if (!enableLog) {
		return ExecuteToolCallImpl(toolName, argumentsJson, outOk, publicToolCall, cancelCallback);
	}

	LogInternalToolRequest(toolName, argumentsJson);
	const auto startTime = std::chrono::steady_clock::now();
	const std::string result = ExecuteToolCallImpl(toolName, argumentsJson, outOk, publicToolCall, cancelCallback);
	const double elapsedMs = std::chrono::duration<double, std::milli>(
		std::chrono::steady_clock::now() - startTime).count();
	LogInternalToolResponse(toolName, result, elapsedMs);
	return result;
}

