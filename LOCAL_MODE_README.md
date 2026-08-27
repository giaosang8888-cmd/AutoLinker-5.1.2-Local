# AutoLinker 纯本地版本说明

## 版本信息
- 原始版本: AutoLinker 5.1.2
- 修改日期: 2026-08-27
- 修改类型: 纯本地化

## 修改概述

本版本移除了所有网络连接功能，使 AutoLinker 成为纯本地工具。

### 已修改的文件

#### 1. `src/WinINetUtil.cpp`
- 移除了所有 HTTP POST/GET 请求实现
- 所有网络请求现在返回错误信息 `[LOCAL MODE] Network requests are disabled in local-only version`

#### 2. `src/TavilyClient.cpp`
- 移除了 Tavily 搜索 API 调用
- 搜索功能返回错误信息 `[LOCAL MODE] Tavily search is disabled in local-only version`

#### 3. `src/WebDocumentClient.cpp`
- 移除了网页内容抓取功能
- 所有抓取请求返回错误信息 `[LOCAL MODE] Web document fetch is disabled in local-only version`

#### 4. `src/AIConfigDialog.cpp`
- 禁用了预设 AI 站点下拉菜单（保留了定义但不使用）
- 将"从转发平台获取Key"链接改为静态提示文本 `[本地模式] 请手动配置 API`
- 移除了外部链接打开功能

#### 5. `src/webview/ai_config_dialog.html`
- 标题添加 `[本地模式]` 标识
- Base URL 输入框提示改为 `本地模式：手动输入 API 地址`
- 禁用"使用预设站点新建"按钮
- 禁用"获取模型列表"按钮
- Tavily 配置区域改为提示 `[本地模式] 网络搜索功能已禁用`

## 功能影响

### 已禁用的功能
1. **AI 模型列表获取** - 无法从网络获取可用模型
2. **Tavily 网络搜索** - `search_web_tavily` 工具不可用
3. **网页内容抓取** - `fetch_url` 和 `extract_web_document` 工具不可用
4. **预设站点快捷配置** - 必须手动输入 API 地址

### 仍然可用的功能
1. **手动配置 API** - 可以输入任意兼容的 API 地址和密钥
2. **所有本地文件操作** - 文件读取、搜索、编辑功能正常
3. **IDE 集成功能** - 编译、调试、项目管理等功能正常

## 编译说明

本纯本地版本可以使用标准编译流程：

```bash
# 使用 Visual Studio 或 MSBuild 编译
msbuild AutoLinker.sln /p:Configuration=Release
```

## 如何恢复网络功能

如需恢复网络功能，需要将以下文件替换为原始版本：
- `src/WinINetUtil.cpp`
- `src/TavilyClient.cpp`
- `src/WebDocumentClient.cpp`
- `src/AIConfigDialog.cpp` (恢复预设站点部分)
- `src/webview/ai_config_dialog.html`

## 注意事项

1. 此版本不会发送任何网络请求
2. 适用于需要完全离线使用或对网络安全有要求的场景
3. AI 功能仍可使用，只需手动配置本地可访问的 API 服务器
