# OpenArkCompiler (ISCAS PLCT Fork)

## 一些技巧和使用脚本

我们(软件所智能研究中心 & 重德智能)提供了一些实用的脚本来加速上手和学习：

初始化：直接运行 [tools/download_and_setup.sh](tools/download_and_setup.sh)
就可以一键设置环境和编译成功（在 Ubuntu 16.04 LTS 环境下）。

知乎上的 [方舟编译器学习笔记](https://zhuanlan.zhihu.com/openarkcompiler)
是软件所PLCT团队成员的学习分析笔记，通过连载的形式，先发表于知乎，
相关的代码和详细分析内容托管在本仓库中。目前大概每天更新一篇短文，
适合初学者跟随着一起学习。

对于有兴趣参与软件所维护的方舟编译器（本仓库）以及软件所为方舟开发的
[Toy Runtime](https://github.com/isrc-cas/pacific)
的话，欢迎发邮件给项目维护人员 [Wei Wu](mailto:w@droid.ac.cn)

以下为官方的README文档内容：

## 概述

> 面向多设备、支持多语言的统一编程平台。

OpenArkCompiler是来自华为方舟编译器的开源项目。

### OpenArkCompiler 四个技术特点 ###

能够将不同语言代码编译成一套可执行文件，在运行环境中高效执行：
- 支持多语言联合优化、消除跨语言调用开销；
- 更轻量的语言运行时；
- 软硬协同充分发挥硬件能效；
- 支持多样化的终端设备平台

## 开源计划

**编译器框架代码开源**
- 时间：2019年8月 
- 开源范围：编译器IR+中端语言实现
- 开放能力：
   - 框架开源供参考学习，了解方舟编译器架构及框架代码
   - 开发者可构建出完整编译器工具链，支持Java Sample程序编译（非应用）

**后续开源范围**

陆续开源编译器前端、后端、编译优化；完整支持Java程序编译、JavaScript语言应用的编译等。

**计划持续更新...**

## 相关文档

- 架构设计原理
   - [MAPLE IR Design](doc/MapleIRDesign.md)
   - [RC API](doc/RC_API.md)
   - [Naive RC操作插入原理](doc/Naive_RC_Insertion_Description.md)
   - [虚函数表和接口函数表设计介绍](doc/Vtable_Itable_Description.md)
   - [Phase设计介绍](doc/Compiler_Phase_Description.md)

- [环境配置](doc/Development_Preparation.md)

- [开发者指南](doc/Developer_Guide.md)

- [编程规范](doc/Programming_Specifications.md)



## 许可证
- [LICENSE](license/LICENSE)
- [开源软件声明](license/Third_Party_Open_Source_Software_Notice.md)

