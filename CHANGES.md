# AutoLinker 5.1.2 纯本地版本 - 改动明细

## 版本信息
- **原始版本**: AutoLinker 5.1.2
- **修改版本**: AutoLinker 5.1.2-Local
- **修改日期**: 2026-08-27

---

## 改动明细

### 1. WinINetUtil.cpp
**文件路径**: `src/WinINetUtil.cpp`

**改动类型**: 功能禁用

**改动说明**:
- 完全移除了 WinINet HTTP 客户端实现
- 所有 HTTP 请求函数（`PerformPostRequest`、`PerformPostRequestStreaming`、`PerformGetRequest`）改为返回本地模式错误信息
- 返回错误: `[LOCAL MODE] Network requests are disabled in local-only version`

**改动原因**:
- 移除所有外部网络通信能力
- 确保应用无法发送任何 HTTP 请求

---

### 2. TavilyClient.cpp
**文件路径**: `src/TavilyClient.cpp`

**改动类型**: 功能禁用

**改动说明**:
- 移除了 Tavily 搜索 API 的网络调用实现
- `TavilyClient::Search()` 函数改为返回本地模式错误

**改动原因**:
- Tavily 是第三方网络搜索服务，禁用以确保完全离线

---

### 3. WebDocumentClient.cpp
**文件路径**: `src/WebDocumentClient.cpp`

**改动类型**: 功能禁用

**改动说明**:
- 移除了网页内容抓取功能
- `WebDocumentClient::FetchTextUrl()` 函数改为返回本地模式错误

**改动原因**:
- 网页抓取功能会访问外部网站，禁用以确保离线

---

### 4. GameAnalyticsClient.cpp
**文件路径**: `src/GameAnalyticsClient.cpp`

**改动类型**: 功能完全重写

**改动说明**:
- 完全重写了遥测客户端
- 移除了所有网络请求代码（包括 HMAC 签名、gzip 压缩等）
- `Initialize()` / `Shutdown()` / `IsRunning()` 等函数改为空实现
- 所有 `PerformPostRequest` 调用移除
- 不再发送任何分析数据

**改动原因**:
- GameAnalytics 是遥测/分析服务，会收集并发送使用数据
- 完全移除以保护用户隐私

---

### 5. AutoLinker.cpp
**文件路径**: `src/AutoLinker.cpp`

**改动类型**: 功能禁用

**改动说明**:
- 禁用了 `FneCheckNewVersion()` 函数中的 GitHub 版本检查
- 不再调用 `PerformGetRequest` 检查更新
- 仅输出本地模式提示信息

**改动原因**:
- 自动版本检查会访问 GitHub API
- 用户需要手动检查更新，禁用自动检查以避免网络请求

---

### 6. AIConfigDialog.cpp
**文件路径**: `src/AIConfigDialog.cpp`

**改动类型**: UI 修改

**改动说明**:
- 将"从转发平台获取Key"链接改为静态文本提示: `[本地模式] 请手动配置 API`
- 移除了 `ShellExecute` 打开外部链接的功能
- 改为显示 MessageBox 提示

**改动原因**:
- 原链接指向外部网站获取 API Key
- 改为本地提示，引导用户手动配置

---

### 7. AIChatFeature.cpp
**文件路径**: `src/AIChatFeature.cpp`

**改动类型**: 链接移除

**改动说明**:
- 注释掉了 GitHub 相关 URL 常量
- 修改空状态页面，移除外部链接
- 将 MCP Guide 链接改为静态文本提示

**改动原因**:
- 移除指向 GitHub 的外部链接
- 避免用户点击后打开浏览器

---

### 8. ai_config_dialog.html
**文件路径**: `src/webview/ai_config_dialog.html`

**改动类型**: UI 修改

**改动说明**:
- 标题添加 `[本地模式]` 标识
- Base URL 输入框提示改为 `本地模式：手动输入 API 地址`
- 禁用"使用预设站点新建"按钮（设为 disabled）
- 禁用"获取模型列表"按钮（设为 disabled）
- Tavily 配置区域改为提示 `[本地模式] 网络搜索功能已禁用`

**改动原因**:
- 明确告知用户当前为纯本地版本
- 禁用需要网络的功能按钮

---

### 9. ai_chat_history.html
**文件路径**: `src/webview/ai_chat_history.html`

**改动类型**: UI 修改

**改动说明**:
- 标题从 `AUTOLINKER` 改为 `AUTOLINKER [LOCAL]`
- 移除外部链接（GitHub 项目链接）

**改动原因**:
- 明确标识本地模式
- 移除可能的外链

---

## 功能影响对照表

| 功能模块 | 原状态 | 修改后 | 说明 |
|---------|-------|-------|------|
| HTTP 网络请求 | 正常 | 禁用 | 所有请求返回错误 |
| AI API 调用 | 正常 | 禁用 | 需手动配置本地 API |
| 模型列表获取 | 正常 | 禁用 | 按钮已禁用 |
| Tavily 搜索 | 正常 | 禁用 | search_web_tavily 不可用 |
| 网页抓取 | 正常 | 禁用 | fetch_url 不可用 |
| GameAnalytics 遥测 | 正常 | 完全禁用 | 不发送任何数据 |
| GitHub 版本检查 | 自动 | 禁用 | 需手动检查 |
| 外部链接 | 可打开 | 禁用 | 不再打开浏览器 |
| MCP 本地服务 | 正常 | **完整保留** | 127.0.0.1 HTTP 服务 |
| IDE 集成 | 正常 | **完整保留** | 所有功能正常 |
| 本地文件操作 | 正常 | **完整保留** | 所有功能正常 |
| 预设站点配置 | 正常 | **完整保留** | 用户可选择使用 |

---

## MCP 功能完整性确认

以下 MCP 相关文件**未被修改**，功能完整保留：

| 文件 | 功能 |
|------|------|
| `LocalMcpServer.cpp/h` | 本地 HTTP 服务器（127.0.0.1）|
| `LocalMcpInstanceRegistry.cpp/h` | 本地实例注册表 |
| `AIService.cpp` 中的 `BuildPublicToolCatalogJson()` | MCP 工具目录构建 |
| `AIService.cpp` 中的 `ExecutePublicTool()` | MCP 工具执行 |

**说明**: LocalMcpServer 使用 Windows Winsock 创建本地 HTTP 服务器，绑定地址为 `127.0.0.1`，完全在本地运行，不依赖 WinINet 或任何外部网络功能。

---

## 恢复网络功能

如需恢复网络功能，请将以下文件替换为原始版本：
- `src/WinINetUtil.cpp`
- `src/TavilyClient.cpp`
- `src/WebDocumentClient.cpp`
- `src/GameAnalyticsClient.cpp`
- `src/AutoLinker.cpp` (仅恢复版本检查部分)
- `src/AIConfigDialog.cpp` (仅恢复获取Key链接部分)
- `src/AIChatFeature.cpp` (仅恢复GitHub链接部分)
- `src/webview/ai_config_dialog.html`
- `src/webview/ai_chat_history.html`

---

## 编译说明

```bash
# 使用 Visual Studio 打开 AutoLinker.sln
# 选择 Release 配置
# 编译整个解决方案

# 或使用命令行编译
msbuild AutoLinker.sln /p:Configuration=Release /p:Platform=Win32
```

---

**修改者**: Claude (AI Assistant)
**日期**: 2026-08-27
