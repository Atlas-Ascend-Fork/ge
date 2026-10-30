# Precision Comparison

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:54:28.300Z pushedAt=2026-10-28T07:42:19.755Z nl=2 -->

## ge.bufferOptimize

Switch for data cache optimization.

**Parameter values:**

- l1\_optimize: indicates enabling l1 optimization. In the current version, this parameter is invalid and is equivalent to off\_optimize.
- l2\_optimize: (default value) indicates enabling l2 optimization.
- off\_optimize: indicates disabling data cache optimization.

**Usage recommendations:**

It is recommended to enable the data cache optimization feature: enabling data cache optimization improves computation efficiency and performance. However, because some operators may have scenarios that are not considered in their implementation, precision may be affected. Therefore, when a precision issue occurs, you can try disabling data cache optimization. If the precision meets the requirement after disabling the data cache optimization feature, identify the problematic operator and report it to technical support for further analysis and resolution. After the operator issue is resolved, it is recommended to keep the data cache optimization feature enabled.

**Configuration example:**

```c++
{"ge.bufferOptimize", "l2_optimize"};
```

**Required/Optional**: Optional

**Effective level**: session/graph

## ge.exec.enableDump

Whether to enable the dump feature.

- 1: Enables the dump feature, reads the dump file saving path from dump\_path, and raises an exception when dump\_path is None.
- 0: (Default value) Disables the dump feature.

**Configuration example:**

```c++
{"ge.exec.enableDump", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

**Usage constraints:**

- This parameter cannot be set simultaneously with the ge.exec.enableDumpDebug parameter in the global scenario or within the same Session.
- If the "ge.exec.enableDump/ge.exec.enableDumpDebug (choose one of two)" parameter is configured as "1" and “ge.exec.enable\_exception\_dump” is configured as "1" (that is, normal ExceptionDump is enabled), then:
  - For dynamic shape networks, only “ge.exec.enable\_exception\_dump” takes effect.
  - For static shape networks, both “ge.exec.enable\_exception\_dump” and "ge.exec.enableDump/ge.exec.enableDumpDebug (choose one of two)" take effect.

## ge.exec.enableDumpDebug

Whether to enable overflow detection.

- 1: Enables overflow detection. The Dump file saving path is read from ge.exec.dumpPath. An exception is raised when the path is configured as None.
- 0: (Default value) Disables overflow detection.

**Configuration example:**

```c++
{"ge.exec.enableDumpDebug", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

**Usage constraints:**

- This parameter and the ge.exec.enableDump parameter cannot be set at the same time in the global scenario or within the same session.
- If the "ge.exec.enableDump/ge.exec.enableDumpDebug (choose one of two)" parameter is configured as "1" and “ge.exec.enable\_exception\_dump” is also configured as "1" (that is, normal ExceptionDump is enabled), then:
  - For dynamic shape networks, only “ge.exec.enable\_exception\_dump” takes effect.
  - For static shape networks, both “ge.exec.enable\_exception\_dump” and "ge.exec.enableDump/ge.exec.enableDumpDebug (choose one of two)" take effect.

## ge.exec.dumpData

Specifies the type of operator dump content. The value can be:

- tensor: (default value) dumps operator data.
- stats: dumps operator statistics, with the result saved in CSV format. Generally, dumping operator data involves a large amount of data, so you can try dumping operator statistics instead.

**Configuration example:**

```c++
{"ge.exec.dumpData", "tensor"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

## ge.exec.dumpDebugMode

Overflow detection mode.

**Parameter values:**

- aicore\_overflow: AI Core operator overflow detection. It detects whether the output contains abnormal extremely large values (such as 65500, 38400, and 51200 under float16) when the operator input data is normal. Once such an issue is detected, you need to analyze the overflow cause based on the actual network requirements and operator logic, and then modify the operator implementation.
- atomic\_overflow: Atomic Add overflow detection. In addition to the AI Core, other modules that involve floating-point computation, such as SDMA, are also checked for overflow issues.
- all: Performs both AI Core operator overflow detection and Atomic Add overflow detection.

<!-- npu="A3,910b" id1 -->
For the following products, only "all" can be configured:

<!-- npu="910b" id2 -->
Atlas A2 products
<!-- end id2 -->

<!-- npu="A3" id3 -->
Atlas A3 products
<!-- end id3 -->
<!-- end id1 -->

**Configuration example:**

```c++
{"ge.exec.dumpDebugMode", "all"};
```

**Required/Optional**: Optional

**Effective level**: global/session

## ge.exec.dumpLayer

Specifies the operators to be dumped. The value is the operator name, and multiple operator names are separated by spaces.

If the input of a specified operator involves the data operator, the data operator information is also dumped.

**Configuration example**:

```c++
{"ge.exec.dumpLayer", "layer1 layer2 layer3"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

## ge.exec.dumpMode

Dump mode, which specifies whether to dump operator input data or output data. The values are as follows:

- input: Dumps only the operator input data.
- output: (Default value) Dumps only the operator output data.
- all: Dumps both the operator input and output data.

**Configuration example:**

```c++
{"ge.exec.dumpMode", "input"};
```

**Usage constraints:**

When configured as all, because some operators modify their input data during execution, such as collective communication operators HcomAllGather and HcomAllReduce, during dumping, the system dumps the operator input before execution and the operator output after execution. As a result, for the same operator, the input and output dump data are written to disk separately, producing multiple dump files. After parsing the dump files, you can determine whether they are input or output based on the file content.

**Required/Optional**: Optional

**Effective level**: Global/session

## ge.exec.dumpPath

Dump file saving path. This parameter must be configured when dump and overflow detection are enabled.

The directory specified by this parameter must be created in advance on the environment where training is started (container or host side), and the running user configured during installation must have read and write permissions on it. Both absolute and relative paths are supported (relative to the current directory when the command is executed).

- An absolute path starts with "/", for example, /home/HwHiAiUser/output.
- A relative path starts directly with the directory name, for example, output.

Dump files are generated in the directory specified by this parameter, that is, in the \{dump\_path\}/\{time\}/\{deviceid\}/\{model\_name\}/\{model\_id\}/\{data\_index\} directory. Taking \{dump\_path\} configured as /home/HwHiAiUser/output as an example, the files are stored in the “/home/HwHiAiUser/output/20200808163566/0/ge\_default\_20200808163719\_121/11/0” directory.

**Required/Optional**: Optional

**Effective Level**: Global/session

## ge.exec.dumpStep

Specifies the iterations for which dump data is collected. Default value: None, which indicates that dump data is generated for all iterations.

Separate multiple iterations with "|", for example, 0|5|10. You can also use "-" to specify an iteration range, for example, 0|3-5|10.

**Configuration example:**

```c++
{"ge.exec.dumpStep", "None"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

## ge.fusionSwitchFile

Path and file name of the configuration file for fusion rule (including graph fusion and UB fusion) switches. The path and file name support uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

The system provides some built-in graph fusion and UB fusion rules, all of which are enabled by default. You can disable specified fusion rules as needed. For the fusion rules that can currently be disabled, see *Graph Fusion and UB Fusion Rule Reference*. However, due to system mechanisms, other fusion rules cannot be disabled.

A configuration file example fusion\_switch.cfg is as follows: on indicates enabling, and off indicates disabling.

Configuration file example:

```json
{
    "Switch":{
        "GraphFusion":{
            "RequantFusionPass":"on",
            "ConvToFullyConnectionFusionPass":"off",
            "SoftmaxFusionPass":"on",
            "NotRequantFusionPass":"on",
            "SplitConvConcatFusionPass":"on",
            "ConvConcatFusionPass":"on",
            "MatMulBiasAddFusionPass":"on",
            "PoolingFusionPass":"on",
            "ZConcatv2dFusionPass":"on",
            "ZConcatExt2FusionPass":"on",
            "TfMergeSubFusionPass":"on"
        },
        "UBFusion":{
            "TbePool2dQuantFusionPass":"on"
        }
    }
}
```
<!-- npu="950" id4 -->
**The Ascend 950PR&950DT products do not support UB fusion, so UBFusion does not need to be configured.**
<!-- end id4 -->

It also supports one-click disabling of fusion rules:

Configuration file example:

```json
{
    "Switch":{
        "GraphFusion":{
            "ALL":"off"
        },
        "UBFusion":{
            "ALL":"off"
            }
    }
}
```
<!-- npu="950" id5 -->
**Ascend 950PR&950DT products do not support UB fusion, so UBFusion does not need to be configured.**
<!-- end id5 -->

Note the following:

1. Disabling certain fusion rules may cause functional issues. Therefore, the one-click disabling here disables only some system fusion rules, not all of them.
2. When fusion rules are disabled with one click, some fusion rules can be enabled at the same time:
    Configuration file example:

    ```json
    {
        "Switch":{
            "GraphFusion":{
                "ALL":"off",
                "SoftmaxFusionPass":"on"
            },
            "UBFusion":{
                "ALL":"off",
                "TbePool2dQuantFusionPass":"on"
            }
        }
    }
    ```

**Configuration example:**

```c++
{"ge.fusionSwitchFile", "/home/test/fusion_switch.cfg"};
```

**Required/Optional**: Optional

**Effective level**: global/session/graph
