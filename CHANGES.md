# 📜 Fjiffyldg - 变更日志 / Changelog

本文件记录 Fjiffyldg 项目的主要版本更新日志。
This document records the release history of the Fjiffyldg project.

---

## [1.0.5] - 2026-06-09
### 重大改进：大幅优化重构 / Major Improvements: Significant Optimization and Refactoring

项目进行了关键性能与稳定性的更新，我们强烈建议您尽快升级至该版本！
The project has undergone critical updates regarding performance and stability. We strongly recommend upgrading to this version as soon as possible!

- **核心重构 / Core Refactoring**：现在完全采用内存映射文件访问优先的策略，若映射失败则回退至文件流访问。
  Now fully adopts a memory-mapped file access-first strategy, falling back to file stream access if mapping fails.
- **性能优化 / Performance Optimization**：减少了数据拷贝次数，且对部分循环判断逻辑做了优化。
  Reduced the number of data copies and optimized some loop judgment logic.
- **错误处理 / Error Handling**：简化了部分接口函数错误处理逻辑，你完全可以只用关心操作是否成功，不必再专门为此考虑更底层的情况了。
  Simplified the error handling logic of some interface functions. You can now focus solely on whether the operation was successful, without needing to consider lower-level details.

---

## [1.0.2] - 2026-05-07
### 🚀 里程碑：正式开源 / Open Source Release

这是 Fjiffyldg 项目的历史性时刻，标志着项目正式从“免费软件”转型为“开源软件”。
This marks a historic moment where Fjiffyldg transitions from "Freeware" to "Open Source Software".

- **📜 许可证变更 / License Update**:
  项目核心代码正式采用 **BSD 3-Clause License**，源代码全面公开。
  The core source code is now officially released under the **BSD 3-Clause License**.
  
- **🔠 名称规范化 / Branding Standardization**:
  项目官方名称正式确定为 **"Fjiffyldg"**，确立统一的品牌标识。
  The official project name is now standardized as **"Fjiffyldg"**.
  
- **⚡ 性能优化 / Performance Boost**:
  重构并优化了分块（Chunk）文件加载算法，显著提升了大文件的初始化速度与内存效率。
  Optimized the chunk file loading algorithm, significantly improving initialization speed and memory efficiency for large files.

---

## [1.0.0] - 2025-12-31
### 🛠️ 初始免费版 / Freeware Phase

项目的初始发布阶段，仅提供二进制文件。
The initial release phase, distributing only binary files.

- **状态 / Status**: 闭源 / Closed Source
- **许可证 / License**: Fjiffyldg Freeware License v1.0.0
- **功能 / Features**: 实现了基础的核心功能闭环 / Core features implemented.