---
name: autolinker-usage
description: >-
  介绍如何使用 AutoLinker 静默 MCP 版，即易语言 AI Agent 支持库和本地 MCP 服务器。
  本版本为纯本地版，移除了所有网络行为（包括 AI Chat UI、右键菜单、版本检查、遥测等）。
  仅保留 Local MCP 功能供外部 Agent 连接使用。内容涵盖安装、本地 MCP 服务器配置、
  协议方法及完整工具参数、外部 Agent（Claude Code / Codex / Cursor）的 MCP 配置、
  多实例路由、无界面命令行编译等。适用于需要纯本地 AI 辅助、易语言源码读写和编译的场景。
---

# AutoLinker 静默 MCP 版使用指南

AutoLinker 静默 MCP 版（`AutoLinker.fne`）是易语言支持库（易语言 IDE 启动时加载的 DLL）。

**本版本的特别说明**：此版本为**纯本地版**，移除了所有网络行为：
- ❌ AI Chat UI 界面（完全禁用）
- ❌ IDE 右键 AI 菜单（完全禁用）
- ❌ AddIn 菜单项（完全禁用）
- ❌ 版本检查更新（完全禁用）
- ❌ GameAnalytics 遥测（完全禁用）
- ❌ Tavily 联网搜索（完全禁用）
- ✅ 本地 MCP 服务器（127.0.0.1:19207）完全保留

IDE 内置 AI 对话功能不可用，但可以通过外部 Agent（如 Claude Code、Codex）连接本地 MCP 服务器来使用 AI 辅助功能。

## 安装

1. 首次安装时将 `AutoLinker.fne` 放入易语言的 `lib` 目录，然后在 IDE 中启用该支持库。
2. 使用易语言 **5.95**。其他易语言版本即使能够运行，也只属于勉强兼容，不提供支持。
3. 不支持 Windows 7。
4. IDE 启动时，AutoLinker 会自动启动本地 MCP 服务器，并将日志写入 IDE 输出窗口和 `autolinker.log`。

AutoLinker 的持久化运行目录是 `{易语言安装目录}\AutoLinker\`；其中"易语言安装目录"
指当前易语言 IDE 主程序（例如 `e5.95.exe`）所在的目录。常用路径如下：

| 用途 | 路径 |
| --- | --- |
| AutoLinker 持久化运行根目录 | `{易语言安装目录}\AutoLinker\` |
| AI 配置 | `{易语言安装目录}\AutoLinker\AIConfig.json` |
| 日志 | `{易语言安装目录}\AutoLinker\Log\` |
| 全局 SKILL | `{易语言安装目录}\AutoLinker\Skills\` |
| 临时缓存 | `%TEMP%\AutoLinker\Cache\` |
| 临时工作区镜像 | `%TEMP%\AutoLinker\workspace-mirror\` |

## 功能概览（静默 MCP 版）

### 可用功能

#### 1. 本地 MCP 服务器

通过外部 Agent（如 Claude Code、Codex、Cursor）连接本地 MCP 服务器来使用 AI 辅助功能。

- 监听 `http://127.0.0.1:19207/mcp`。`19207` 是**固定网关端口**；各 IDE 实例使用
  从 `19208` 开始的内部后端端口。外部工具始终连接 `19207`，不要连接后端端口。
- 启动日志：
  `[AutoLinker][LocalMCP] 本地 MCP 服务已启动：http://127.0.0.1:19207/mcp`
- 协议：JSON-RPC 2.0、Streamable HTTP。`initialize` 可协商 `2025-11-25` /
  `2025-03-26` / `2024-11-05`，未知版本回退到 `2025-11-25`。支持的方法：
  `initialize`、`notifications/initialized`、`ping`、`tools/list`、`tools/call`、
  `DELETE /mcp`。
- 安全：仅绑定 `127.0.0.1`；拒绝包含非空 `Origin` 的浏览器请求；不支持 CORS /
  Bearer token。外部调用不会弹出审批窗口，但仍会执行工具白名单、参数结构校验、
  工作区刷新和源码 SHA-256 CAS。服务器使用 4 个固定工作线程，连接队列最多 32 个，
  过载时返回 HTTP 503。不要将其暴露到局域网或互联网。

#### 2. 鼠标后退键撤销上一次修改

跳转到上一个修改位置，行为与其他 IDE 类似。

### 不可用功能（已禁用）

以下功能在本版本中**不可用**：

- AI Agent 对话页（内置 AI 对话界面）
- 右键 AI 菜单（代码编辑器内的 AI 优化、翻译等功能）
- AddIn 菜单项
- 版本自动检查和更新
- Tavily 联网搜索
- e-packager 自动下载更新

如需使用这些功能，请使用官方标准版 AutoLinker。

## 外部 Agent 配置

### Claude Code

在 `~/.claude.json` 或工程 `.mcp.json` 中配置：

```json
{ "mcpServers": { "AutoLinker": { "type": "http", "url": "http://127.0.0.1:19207/mcp" } } }
```

### Gemini CLI

在 `~/.gemini/settings.json` 中配置：

```json
{ "mcpServers": { "AutoLinker": { "type": "http", "url": "http://127.0.0.1:19207/mcp" } } }
```

### Codex

在 `~/.codex/config.toml` 中配置：

```toml
[mcp_servers.AutoLinker]
url = "http://127.0.0.1:19207/mcp"
```

### Cursor / Windsurf

MCP 设置 → 名称 `AutoLinker`，类型 `http` / `streamable_http`，URL `http://127.0.0.1:19207/mcp`。

### Antigravity CLI

`%USERPROFILE%\.gemini\antigravity\mcp_config.json`。

## 多实例路由

同时打开多个 IDE 时，先调用 `list_instances` 查看实例（工程路径和当前页面），再使用
`instance_id` 调用 `select_instance`。选择结果按 `Mcp-Session-Id` 隔离。如果选中的
实例关闭，AutoLinker 会要求重新选择，而不会静默切换到其他工程。如果占用 `19207`
的 IDE 退出，仍存活的实例会接管网关，客户端继续重连同一地址即可。

## 代码读取技术原理

AutoLinker 不直接解析或修改加密的 `.e` 文件。准备完整工作区时，它先从当前 IDE 内存
工程导出临时快照（因此能包含尚未保存的修改），再调用 e-packager 的 `unpack` 命令，
将快照解包到 `%TEMP%/AutoLinker/workspace-mirror/` 下的实例专用目录。`list_files`、
`search_code`、`read_file` / `read_files` 和 `read_code_item` 读取的都是这个文本镜像。

完整镜像不只包含当前工程源码：

| 镜像目录 | 可读取的内容 |
| --- | --- |
| `src/` | 当前 `.e` 工程解包后的程序集、类、固定表和窗口相关文件 |
| `ecom/` | 当前工程引用的 EC 模块解包后的源码，可用于查找模块命令的实现和调用方式 |
| `elib/` | 当前工程所用支持库的公开信息，包括支持库通过 `GetNewInf` 等公开描述提供的命令、数据类型和接口 |
| `header/` | e-packager 生成的其他声明或头部参考信息 |

`ecom/`、`elib/` 和 `header/` 只用于读取和搜索，不能通过源码写入工具修改。`elib/` 展示
的是支持库公开接口，不是对 `.fne` 内部 C/C++ 实现的反编译。修改当前工程时，AutoLinker
只允许把可写的 `src/` 文本映射回 IDE 程序项，并通过真实页哈希防止覆盖较新的修改。

## 读写模型（外部 Agent 必须了解）

- 外部 MCP 会话在首次读写前必须调用一次 `refresh_workspace_mirror`。`mode` 可取 `auto` / `main_only` / `full`。镜像
  由 e-packager 解包到 `%TEMP%/AutoLinker/workspace-mirror/`，包含未保存的修改，且
  不会触碰源码目录。
- 读取工具使用镜像相对路径（`list_files`、`search_code`、`read_file`、
  `read_files`、`read_code_item`）。大文件会返回 `next_source_byte_offset` 供继续分页；
  后续请求应传回上一页非零的 `mirror_generation`。
- 编辑前，调用 `read_real_file` 获取分页视图和 `code_hash`（CAS 基线）。写入工具通过
  `file_path` 定位目标，并映射回 IDE 程序项后直接写入 IDE，不会在写入时重新编译。
- 写入操作必须携带 SHA-256 `expected_base_hash`（恢复时使用
  `expected_current_hash`），以防止基于过期版本覆盖。
- `src/*.xml` 是易语言原生窗口 UI 文件，仅支持读取/搜索。当前工具不支持修改窗口或控件
  的位置，大小、层级和属性，不支持添加、删除控件，也不支持新增、删除或修改控件的事件
  绑定。不要尝试写入窗口 XML，也不要声称已经完成这些界面修改。
- 窗口程序集代码仍可通过对应的 `src/*.txt` 编辑，包括修改已有控件事件子程序的代码实现；
  但新增一个事件子程序不代表已经建立控件事件绑定。固定表（常量、全局变量、DLL 声明、
  数据类型）可通过对应路径编辑，程序集变量会以 IDE 可接受的形式写回。
- 需要了解模块或支持库时，使用 `list_files` / `search_code` / `read_files` 检索 `ecom/`
  和 `elib/`。不要因为它们出现在镜像中就将其当作当前工程的可写源码。

## 工具集（`tools/list`）

需要查询、生成或排查 MCP 请求时，必须读取
[完整 MCP API 参考](references/mcp-api.md)。该参考以当前代码中的公开目录和参数校验为准，
包含协议级方法、HTTP 会话操作、21 个 `tools/list` 工具、所有参数及嵌套参数、默认值、
取值范围、调用前置条件、关键返回字段和错误处理。

工具按用途分为：工作区刷新与镜像读取、真实页 CAS 编辑、IDE 状态与编译、公网页面访问、
多实例路由。标准调用顺序是 `initialize` → 必要时 `list_instances` / `select_instance` →
`get_current_eide_info` → `refresh_workspace_mirror` → 读取/编辑/编译工具。不要根据本节摘要
猜测参数；实际调用前查阅子文档中的对应工具条目。

## 无界面命令行编译

推荐方式：通过 `AutoLinkerTest headless-compile` 启动 `e.exe`。该命令会关闭启动弹窗、
隐藏 IDE、调用 `compile_with_output_path`，并输出 JSON 结果。启动器支持多个进程同时调用：
同一 `.e` 工程按调用顺序排队，不同工程可并行编译；相同输出路径通过跨进程锁避免同时写入：

```powershell
.\bin\fne_release\AutoLinkerTest.exe headless-compile `
  "C:\path\to\e571.exe" "D:\demo\demo.e" "D:\demo\build\demo.exe" `
  --target auto --static --result "D:\demo\build\compile-result.json" --timeout 120
```

`--target` 可取 `auto` | `win_exe` | `win_console_exe` | `win_dll` | `ecom`。
`--static` 仅适用于 EXE/DLL。省略 `--result` 时，每次调用生成独立的
`<输出文件>.headless.<invocation-id>.json`，并原子更新兼容文件
`<输出文件>.headless.json`。`{易语言目录}\AutoLinker\Log\headless_compile_last.json`
也是原子更新的最近一次结果，不能用于区分并发调用。显式传入 `--result` 时，各调用必须使用
不同结果路径。

也可以直接驱动主程序（仅限无界面编译；处理早期弹窗和同一工程并发启动时仍优先使用启动器）：

```powershell
"C:\path\to\e571.exe" "D:\demo\demo.e" `
  --autolinker-headless-compile `
  --autolinker-output "D:\demo\build\demo.exe" `
  --autolinker-target auto `
  --autolinker-result "D:\demo\build\compile-result.json" `
  --autolinker-invocation-id "compile-001"
```

`AutoLinkerTest.exe` 还提供以下测试和自检子命令：
`headless-compile`、`mock-mcp-stdio`、`mcp-self-test`、
`version-text`、`version-compare`、`deepseek-model-test`、
`openai-chat-test`、`openai-responses-test`、`gemini-model-test`、
`claude-model-test`、`linker-out` / `linker-krnln` / `between-dashes`。

## 构建和测试 AutoLinker（贡献者）

构建 `fne_release` / x86（VS2022 VC++、ISO C++20）：

```powershell
MSBuild.exe ..\AutoLinker.vcxproj /t:Build "/p:Configuration=fne_release;Platform=Win32" /m
```

`MSBuild.exe` 的位置因计算机和 VS 版本而异，请先定位可执行文件。

测试编译后的支持库：先关闭易语言主程序以释放 `AutoLinker.fne` 文件锁，再将
`AutoLinker.fne` 复制到 IDE 的 `lib` 目录；然后打开 `eproj/` 下的 `.e` 文件并测试
相关功能。加载时 MCP 会在端口 19207 上启动。传递中文页面名称时使用 **UTF-8**。
不依赖 IDE 的逻辑（例如模块本地解析）可使用 `AutoLinkerTest` 工程测试。

## 兼容性和问题反馈前提

回答或排查 AutoLinker 问题前，按以下顺序确认环境：

1. 确认操作系统不是 Windows 7。
2. 确认使用易语言 **5.95**；其他版本不在支持范围内。
3. 确认已经更新到**最新版 AutoLinker 静默 MCP 版**。
4. 确认没有同时加载其他第三方插件或支持库。遇到闪退、异常行为或疑似冲突时，先卸载
   或禁用其他插件，仅保留 AutoLinker 后重新测试。
5. 只有在上述条件全部满足且问题仍可复现时，再按 AutoLinker 问题或缺陷继续排查，并
   提供易语言版本、AutoLinker 版本、复现步骤和相关日志。

## 常见问题

### 为什么外部 Agent 调不通 MCP？

此类现象优先按外部 Agent 工具的 MCP 配置或会话权限问题排查：

1. 确认 AutoLinker 启动日志已经显示本地 MCP 服务运行在
   `http://127.0.0.1:19207/mcp`。
2. 确认外部工具的 MCP 配置指向固定网关端口 `19207`，而不是 `19208` 及以上的内部
   后端端口。
3. 配置 MCP 后，检查外部工具是否还要求在**当前会话**中单独启用或授权 AutoLinker
   MCP。有些工具只保存服务器配置，不会自动为每个会话开放 MCP 权限。
4. 确认外部工具能够直接列出并调用 AutoLinker MCP 工具。如果它转而编写 `.py`、
   PowerShell 脚本或 CMD 命令来请求 MCP，通常说明其原生 MCP 没有配置好，没有启用，
   或当前会话没有权限；先按该外部工具的文档修正配置和授权。

### e-packager 无法使用怎么办？

1. 确认 e-packager 已正确安装到 `{易语言安装目录}\tools\e-packager.exe`。
2. 如果无法自动下载，可以手动从 GitHub 获取：
   https://github.com/aiqinxuancai/e-packager/releases
3. 将下载包解压，确保 `e-packager.exe` 位于正确位置。
4. 重启易语言 IDE，再调用或重试 `refresh_workspace_mirror`。

## 参考资料

- AutoLinker 静默 MCP 版项目：https://github.com/giaosang8888-cmd/AutoLinker-5.1.2-Local
- e-packager：https://github.com/aiqinxuancai/e-packager
