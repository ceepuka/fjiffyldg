# fjiffyldg

> 🆓 Freeware | 📅 Planned Open Source: 2026-06-30 | 📚 [English](README.md)

**fjiffyldg** 是一个跨平台的轻量级大文件高效加载处理开发库。它提供了随机访问TB级大文件、智能文件行索引管理（完全兼容UTF-16/32宽字符文本文件）、UTF-8文本快速检测等优秀的底层功能。设计理念是**满足刚需，不留冗余，无复杂内存管理**。

<div align="center">
  
![Status](https://img.shields.io/badge/Status-Freeware-blue)
![Open Source Plan](https://img.shields.io/badge/Open_Source-2026--06--30-green)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Android-orange)

</div>

## ✨ 核心特性

- ✅ **双接口支持**: 兼容C/C++ API，C++完全RAII化
- ✅ **超大文件处理**: 理论限制<8EB，实际应用无上限
- ✅ **智能行索引**: 直接行定位百万文件行，任意位置快速查找所在行
- ✅ **高性能跨平台**: 基于U++框架，静态链接零依赖
- ✅ **编码全支持**: UTF-8/16/32宽字符文本文件完整兼容

## 📥 快速开始

### 下载安装
```bash
# 当前可从Release页面下载预编译库
https://github.com/ceepuka/fjiffyldg/releases
```

### 基础用法

#### **C API 示例**
```c
#include "fjiffyldg.h"

int main() {
    fjiffyldg_ptr fm = fjiffyldg_create();
    if (!LoadAndScanFile(fm, "test.txt")) {
        unsigned int len = 5;
        const char* s = ReadFileData(fm, 0, len);
        if (s) printf("%.5s\n", s);
    }
    fjiffyldg_clear(fm);
    return 0;
}
```

#### **C++ RAII 示例**
```cpp
#include "fjiffyldg.h"
using namespace Fjiffyldg;

int main() {
    Fjiffyldg ldg;  // RAII自动管理
    fjiffyldg_ptr fm = ldg.GetFjiffyldgHandle();
    
    if (!LoadAndScanFile(fm, "test.txt")) {
        unsigned int len = 5;
        const char* s = ReadFileData(fm, 0, len);
        if (s) printf("%.5s\n", s);
    }
    return 0;  // 自动清理
}
```

### 常见平台依赖库

| 平台 | 需要链接的库 |
|------|--------------|
| **Windows** | `kernel32 user32 ole32 oleaut32 uuid ws2_32 advapi32 shell32 winmm mpr` |
| **Linux** | `pthread dl rt` |
| **macOS** | `pthread` |
| **Android** | `atomic` 和 `cpufeatures (static)` |
| **Solaris** | `posix4 dl socket nsl` |

## ⚖️ 许可证状态

### 📅 关键时间点
- **2025-12-31**: 发布免费软件 v1.0.0
- **2026-06-30**: 转换为 BSD 3-Clause 开源许可证
- **之后**: 完全开源

### 📊 权限对比表
| 权限 | 当前 (免费软件) | 未来 (BSD 3-Clause) |
|------|----------------|---------------------|
| **商业使用** | ✅ 允许 | ✅ 允许 |
| **查看源码** | 部分公开* | ✅ 完全公开 |
| **修改代码** | ❌ 不允许 | ✅ 允许 |
| **重新分发** | ✅ 二进制 | ✅ 源码/二进制 |
| **提交PR** | ❌ 不接受 | ✅ 欢迎 |

> *注：U++框架源码当前可访问，项目原创代码暂不公开*

## 🛠️ 技术架构

### 构建框架
本项目基于 **U++ Framework (Ultimate++)**，采用源码级集成：

```plaintext
构建流程：
1. 导出所需的 U++ Core 模块源文件
2. 编译精简的 U++ 核心库
3. 编译 fjiffyldg 主模块
4. 静态链接生成独立库
```

### 架构优势
- 🔧 **零依赖部署**: 无需系统U++安装，真正独立
- 📦 **轻量级**: 仅导出必要模块，减小库体积
- 🌍 **跨平台一致**: 相同代码在Windows/Linux/macOS/Android运行
- ⚡ **高性能**: 静态链接，无运行时开销

## 📜 许可证说明

### 双重许可证结构
| 组件 | 当前许可证 | 未来许可证 | 状态 |
|------|------------|------------|------|
| **U++ 框架代码** | BSD License | BSD License | 已公开 |
| **fjiffyldg 原创代码** | Freeware License | BSD 3-Clause | 2026-06-30公开 |

### 重要文件
- **[LICENSE](LICENSE)** - 本项目免费软件许可证
- **[UPP-LICENSE.md](UPP-LICENSE.md)** - U++框架许可证声明
- **[README.md](README.md)** - 英文文档
- **[README_ZH.md](README_ZH.md)** - 中文文档

## 🤝 参与贡献

### 当前阶段 (免费软件)
- 🐛 **[报告问题](https://github.com/ceepuka/fjiffyldg/issues)** - 遇到Bug或异常行为
- 💡 **[功能建议](https://github.com/ceepuka/fjiffyldg/wiki)** - 提出改进或新功能想法
- 📖 **文档改进** - 帮助完善使用文档

### 开源后 (2026-06-30起)
- 🔀 **提交PR** - 代码改进和新功能
- 🧪 **编写测试** - 提升代码质量
- 🌐 **多语言支持** - 翻译和国际化
- 📚 **示例代码** - 丰富使用示例

## 🙏 致谢

衷心感谢 **U++ Framework (Ultimate++)** 开发团队创造了如此优秀、高效的跨平台C++框架。

### U++框架的优势
- 🚀 **高性能**: 精心优化的底层实现
- 📐 **优雅设计**: "少写代码，多做事情"的理念
- 🔧 **完备工具链**: TheIDE提供完整开发体验
- 🌍 **真正跨平台**: 一套代码多处运行

### 鼓励支持
我们鼓励所有开发者：
- 🌐 访问 [U++官方网站](https://www.ultimatepp.org/)
- 💻 关注 [GitHub仓库](https://github.com/ultimatepp/ultimatepp)
- 🛠️ 在适合的项目中使用这个优秀的框架

## 📞 联系支持

- **项目主页**: https://github.com/ceepuka/fjiffyldg
- **创始人邮箱**: 2641451398@qq.com
- **问题反馈**: [Issues页面](https://github.com/ceepuka/fjiffyldg/issues)
- **功能讨论**: [Discussions页面](https://github.com/ceepuka/fjiffyldg/wiki)
- **文档**: [中文文档](README_ZH.md) | [English](README.md)

---

<div align="center">
  
**版权声明**: © 2025 Du Jie (@ceepuka) - 本软件当前为免费软件，计划于2026年6月30日开源。

[![Star](https://img.shields.io/github/stars/ceepuka/fjiffyldg?style=social)](链接)
[![Watch](https://img.shields.io/github/watchers/ceepuka/fjiffyldg?style=social)](链接)

</div>