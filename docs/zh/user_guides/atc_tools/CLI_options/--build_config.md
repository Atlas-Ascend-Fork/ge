# --build\_config

## 产品支持情况

全量芯片支持。

## 功能说明

指定图编译阶段生成源码编译动态库时使用的make命令模板，用于自定义交叉编译器、编译选项、预处理选项、链接选项等参数。GE会在执行编译时自动补充生成的Makefile文件，用户无需指定Makefile文件。

## 关联参数

仅在[--mode](--mode.md)=7生成\*.om2格式离线模型的源码编译阶段生效。

## 参数取值

**参数值：**make命令模板，用于指定生成源码编译阶段使用的make命令、make选项以及编译相关的变量。

**参数值格式：**参数值需放在双引号中，格式如下：

```bash
--build_config="<make命令> [make选项] [变量赋值...]"
```

- <make命令\>：必须为make或以/make结尾的make命令绝对路径，例如make、/usr/bin/make。
- \[make选项\]：可选项，支持常见make选项，例如-s、-j16。
- \[变量赋值\]：可选项，格式为变量名=变量值，带空格的变量值可使用单引号或双引号包裹，例如CXXFLAGS='-O2 -fPIC'。仅支持配置以下make变量：

    CXX、CXXFLAGS、CPPFLAGS、LDFLAGS、LDLIBS

**参数值约束：**

- 不支持任意shell命令，仅支持make命令模板。
- 不支持用户指定Makefile文件，例如-f、--file、--makefile等。

## 推荐配置及收益

当编译环境为x86\_64、运行环境为aarch64且生成\*.om2离线模型（[--mode](--mode.md)=7）时，若需使用开发套件包内置的交叉编译工具（用于在 x86\_64环境编译aarch64环境的g++源码），请配置[--host\_env\_cpu](--host_env_cpu.md)和[--host\_env\_os](--host_env_os.md)参数；其他情况推荐使用--build\_config配置。

## 示例

```bash
atc --build_config="make -s -j16 CXX=/usr/bin/aarch64-linux-gnu-g++ CXXFLAGS='-O2 -fPIC'"
```

## 依赖约束

无。
