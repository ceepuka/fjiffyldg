# Fjiffyldg

> 🆓 Freeware | 🚀 Open Source | 📅 Opened: 2026-05-07 | 📚 [Chinese](README_ZH.md)

**Fjiffyldg** is a cross-platform lightweight library for efficient large file loading and processing. It provides excellent low-level capabilities including random access to terabyte-scale files, intelligent file line index management (fully compatible with UTF-16/32 wide-character text files), and fast UTF-8 text detection. The design philosophy is **meet essential needs, leave no redundancy, with no complex memory management**.

<div align="center">
  
![Status](https://img.shields.io/badge/Status-Open_Source-brightgreen)
![License](https://img.shields.io/badge/License-BSD--3--Clause-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Android-orange)

</div>

## ✨ Core Features

- ✅ **Dual Interface Support**: Compatible C/C++ API, C++ fully RAII-enabled
- ✅ **Massive File Processing**: Theoretical limit <8EB, practically unlimited in applications
- ✅ **Intelligent Line Indexing**: Direct line positioning for million-line files, fast location finding at any position
- ✅ **High-Performance Cross-Platform**: Based on U++ framework, statically linked with zero dependencies
- ✅ **Full Encoding Support**: Complete compatibility with UTF-8/16/32 wide-character text files

## 📥 Quick Start

### Download & Install
```bash
# Currently available as precompiled libraries from the Releases page
https://github.com/ceepuka/fjiffyldg/releases
```

### Basic Usage

#### **C API Example**
```c
#include <stdio.h>
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

#### **C++ RAII Example**
```cpp
#include <iostream>
#include <string>
#include "fjiffyldg.h"
using namespace Fjiffyldg;

int main() {
    Fjiffyldg ldg;  // RAII automatic management
    fjiffyldg_ptr fm = ldg.GetFjiffyldgHandle();
    
    if (!LoadAndScanFile(fm, "test.txt")) {
        unsigned int len = 5;
        const char* s = ReadFileData(fm, 0, len);
        if (s) std::cout << std::string(s, 5) << endl;
    }
    return 0;  // Automatic cleanup
}
```

### Platform Library Dependencies

| Platform | Libraries to Link |
|----------|-------------------|
| **Windows** | `kernel32 user32 ole32 oleaut32 uuid ws2_32 advapi32 shell32 winmm mpr` |
| **Linux** | `pthread dl rt` |
| **macOS** | `pthread` |
| **Android** | `atomic` and `cpufeatures (static)` |
| **Solaris** | `posix4 dl socket nsl` |

## ⚖️ License Status

### 📊 Permission Table
| Permission | Current (BSD 3-Clause) |
|------------|----------------------|
| **Commercial Use** | ✅ Allowed |
| **View Source Code** | ✅ Fully public |
| **Modify Code** | ✅ Allowed |
| **Redistribution** | ✅ Source/Binary |
| **Submit PR** | ✅ Welcome |

## 🛠️ Technical Architecture

### Build Framework
This project is based on **U++ Framework (Ultimate++)** with source-level integration:

```plaintext
Build Process:
1. Export required U++ Core module source files
2. Compile minimal U++ core library
3. Compile fjiffyldg main module
4. Statically link to generate standalone library
```

### Architecture Advantages
- 🔧 **Zero-Dependency Deployment**: No system U++ installation required, truly independent
- 📦 **Lightweight**: Only exports necessary modules, reduces library size
- 🌍 **Cross-Platform Consistency**: Same code runs on Windows/Linux/macOS/Android
- ⚡ **High Performance**: Statically linked, no runtime overhead

## 📜 License Information

### License Structure
| Component | License | Status |
|-----------|---------|--------|
| **Fjiffyldg Original Code** | BSD 3-Clause | ✅ Fully Open Source |

### Important Files
- **[LICENSE](LICENSE)** - This project's freeware license
- **[UPP-LICENSE.md](UPP-LICENSE.md)** - U++ framework license statement
- **[README.md](README.md)** - English documentation
- **[README_ZH.md](README_ZH.md)** - Chinese documentation

## 🤝 Contributing

### We Welcome Contributions
- 🔀 **Submit PRs** - Code improvements and new features
- 🧪 **Write Tests** - Enhance code quality
- 🌐 **Multi-language Support** - Translations and internationalization
- 📚 **Example Code** - Enrich usage examples

## 🙏 Acknowledgments

Sincere thanks to the **U++ Framework (Ultimate++)** development team for creating such an excellent and efficient cross-platform C++ framework.

### Advantages of U++ Framework
- 🚀 **High Performance**: Carefully optimized low-level implementation
- 📐 **Elegant Design**: Philosophy of "write less code, do more"
- 🔧 **Complete Toolchain**: TheIDE provides full development experience
- 🌍 **Truly Cross-Platform**: One codebase runs everywhere

### Encouraging Support
We encourage all developers to:
- 🌐 Visit [U++ Official Website](https://www.ultimatepp.org/)
- 💻 Follow [GitHub Repository](https://github.com/ultimatepp/ultimatepp)
- 🛠️ Use this excellent framework in suitable projects

## 📞 Contact & Support

- **Project Homepage**: https://github.com/ceepuka/fjiffyldg
- **Founder Email**: 2641451398@qq.com
- **Issue Reporting**: [Issues Page](https://github.com/ceepuka/fjiffyldg/issues)
- **Feature Discussion**: [Discussions Page](https://github.com/ceepuka/fjiffyldg/wiki)
- **Documentation**: [Chinese](README_ZH.md) | [English](README.md)

---

<div align="center">
  
**Copyright Notice**: © 2025-2026 Du Jie (@ceepuka) - The Fjiffyldg Project
All rights reserved.

[![Star](https://img.shields.io/github/stars/ceepuka/fjiffyldg?style=social)](https://github.com/ceepuka/fjiffyldg)
[![Watch](https://img.shields.io/github/watchers/ceepuka/fjiffyldg?style=social)](https://github.com/ceepuka/fjiffyldg)

</div>
