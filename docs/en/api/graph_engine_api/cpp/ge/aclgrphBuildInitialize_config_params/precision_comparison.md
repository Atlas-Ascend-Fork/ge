# Precision Comparison

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:40:12.528Z pushedAt=2026-10-28T07:40:22.200Z nl=2 -->

## BUFFER\_OPTIMIZE

Data cache optimization switch. The actual options parameter corresponding to this parameter is `ge.bufferOptimize`.

**Parameter values:**

- **l1\_optimize**: Enables l1 optimization. In the current version, this parameter is invalid and is equivalent to **off\_optimize**.
- **l2\_optimize**: (Default value) Enables l2 optimization.
- **off\_optimize**: Disables data cache optimization.

Here, l1 refers to L1 Buffer, a general-purpose internal storage that is a relatively large data transfer area within the AI Core. It can temporarily store data that needs to be used repeatedly in the AI Core, thereby reducing the number of reads and writes from the bus. l2 refers to L2 Buffer, which represents external storage. The AI Core needs to load data from external storage into internal storage to complete the corresponding computation.

**Usage suggestions:**

It is recommended to enable the data cache optimization feature. Enabling data cache optimization can improve computation efficiency and performance. However, because some operators may have scenarios that are not considered in their implementation, this may affect precision. Therefore, when a precision issue occurs, you can try disabling data cache optimization. If the precision meets the requirement after disabling the data cache optimization feature, you need to identify the problematic operator and report it to technical support for further analysis and resolution. After the operator issue is resolved, it is recommended to keep the data cache optimization feature enabled.

**Configuration example:**

```c++
{ge::ir_option::BUFFER_OPTIMIZE, "l2_optimize"}
```

**Applicable products:**

Supported on all chips.

## FUSION\_SWITCH\_FILE

The actual **options** parameter corresponding to this parameter is `ge.fusionSwitchFile`.

Path and filename of the fusion switch configuration file. The path and filename support uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

The system provides some built-in graph fusion and UB fusion patterns, all of which are enabled by default. You can disable specified fusion patterns as needed. For the fusion patterns that can currently be disabled, see *Graph Fusion and UB Fusion Rule Reference*. However, due to system mechanisms, other fusion patterns cannot be disabled.

<!-- npu="950" id1 -->
**Ascend 950PR&950DT series products do not support UB fusion**.
<!-- end id1 -->

**Configuration example:**

Configuration file example **_fusion\_switch.cfg**, where **_on** indicates enabled and **off** indicates disabled.

Configuration example:

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
        }
    }
}
```

<!-- npu="950" id2 -->
**When UB fusion is not supported, UBFusion can be removed from the above configuration file.**
<!-- end id2 -->

It also supports one-click disabling of fusion patterns:

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

<!-- npu="950" id3 -->
**If UB fusion is not supported, UBFusion can be deleted from the preceding configuration file.**
<!-- end id3 -->

Note the following:

1. Disabling certain fusion patterns may cause functional issues. Therefore, the one-click disabling here disables only some of the system fusion patterns, not all of them.
2. When using one-click disabling of fusion patterns, you can enable some fusion patterns at the same time:

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

    <!-- npu="950" id4 -->
    **If UB fusion is not supported, UBFusion can be deleted from the preceding configuration file.**
    <!-- end id4 -->

**Applicable products:**

Supported on all chips.
