# --export\_compile\_stat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:03:58.322Z pushedAt=2026-10-27T07:43:05.589Z -->

## Product Support

All chips are supported.

## Function Description

Configures **whether to generate** the result file fusion\_result.json of operator fusion information (including graph fusion and UB fusion) during graph compilation.

This file records the fusion rules used during graph compilation. In the file:

- session\_and\_graph\_id\__xx\_xx_: indicates the thread and graph number to which the fusion result belongs.
- graph\_fusion: indicates graph fusion.
- ub\_fusion: indicates UB fusion

    <!-- npu="950" id1 -->
    **The Ascend 950PR&950DT products do not support UB fusion, so this information is not generated**.
    <!-- end id1 -->

- match\_times: indicates the number of times fusion rules are matched during graph compilation.
- effect\_times: indicates the number of times they actually take effect.
- repository\_hit\_times: the number of hits in the optimized UB fusion knowledge base

    <!-- npu="950" id2 -->
    **The Ascend 950PR&950DT products do not support UB fusion, so this information is not generated**.
    <!-- end id2 -->

## Related Parameters

This parameter is used to generate operator fusion information, while the [--fusion\_switch\_file](--fusion_switch_file.md) parameter can disable specified fusion rules. Disabled fusion rules are not presented in the fusion\_result.json file.

## Parameter Values

- 0: Do not generate the operator fusion information result file.
- 1: (Default) Generate the operator fusion information result file when the program exits normally.
- 2: Generate the operator fusion information result file when graph compilation is complete. That is, if graph compilation is complete, the operator fusion information result file is generated even if the program is interrupted early.

If the ASCEND\_WORK\_PATH environment variable is not set, the result file is generated in the current directory where the atc command is executed by default. If the ASCEND\_WORK\_PATH environment variable is set, the result file is saved to `$ASCEND_WORK_PATH/FE/${process ID}/fusion_result.json`. For details about the environment variable, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## Example

```bash
atc --export_compile_stat=1 ...
```

## Constraints

None.
