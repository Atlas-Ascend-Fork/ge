# --h2d\_overlapped\_with\_compute

<!-- md-trans-meta sourceCommit=17c885a7f6b17398067f092afcfd15e2585329ed translatedAt=2026-11-16T13:57:17.698Z pushedAt=2026-11-17T08:48:27.893Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Configures whether the H2D copy of the model input overlaps with model computation.

This parameter is passed to GE as a graph compilation option. GE first filters the inputs that can participate in this function, and sorts them by the position where each input is first used:

- The smaller the topology order of the first consumption node, the earlier the input is sorted.
- When the first consumption nodes are the same, the inputs are sorted in ascending order by input index.

After sorting, each input initially corresponds to an independent H2D copy. Next, GE merges adjacent inputs into several copy groups. The inputs in the same group are copied to the Device in batches through a single H2D copy instruction.

The first group completes the copy before the model starts computing; the remaining groups are copied during model computation and overlap with model computation.

Note that **this parameter is currently a reserved function and is not recommended for configuration**. The feature takes effect only when the input at execution is Host memory (the H2D copy source is on the Host side). However, the input of the existing ACL model execution API (such as aclmdlExecute) is fixed in Device memory handling and cannot pass in Host input. Therefore, models compiled through atc cannot enable this feature end-to-end for now. After the ACL API exposure of Host input passing is available in the future, this parameter can be used normally.

## Related Parameters

None.

## Parameter Values

- "0": (default value) Disables overlap. The model starts computing only after all inputs complete the H2D copy.
- "1": Enables automatic planning. GE automatically groups the inputs and copies partial input during model computation.
- Comma-separated group boundaries, for example "44,64,68": manually specifies how the inputs are grouped.

    The manually configured numbers indicate the group boundary of the sorted input sequence. For example, "44,64,68" indicates that the inputs are divided into four copy groups: [0,44), [44,64), [64,68), and [68,N). Among them, [0,44) completes the copy before the model starts computing, and the remaining three groups perform the copy during model computation.

    If only one boundary is needed, it can be configured as: `44,` or `,44`.

    Both forms indicate that [0,44) completes the copy before the model starts computing, and [44,N) performs the copy during model computation.

    Note that **the boundary numbers are based on the sorting result of GE for the candidate inputs, not the input index of the model**. GE will pass through the INFO log to print the final group boundary and copy interval used:

    ```text
    [InputH2DOverlap] H2D overlap groups: mode=auto, boundaries=[44,64,68]
    ```

    During manual configuration, mode is displayed as manual:

    ```text
    [InputH2DOverlap] H2D overlap groups: mode=manual, boundaries=[44,64,68]
    ```

## Recommended Configuration and Benefits

Applies to scenarios with static shape and a single-stream graph, where the model contains many Host inputs, the H2D copy overhead is large, and the front-end computation of the model can cover partial input copy.

It is recommended to first configure it as "1" to let GE perform automatic planning, and check the actual grouping result through the INFO log above.

If the profiling trace shows that the model computation has obvious H2D waiting, it indicates that the waited copy group is too long. In this case, manually adjust the group boundary to reduce the number of copies in this group.

> Note: The GE INFO log is set through the environment variable `ASCEND_GLOBAL_LOG_LEVEL`. The value `1` indicates the INFO level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR). For details, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/master/docs/en/env-vars/README.md).

## Example

- Automatic planning:

    ```bash
    atc --h2d_overlapped_with_compute=1 --model=model.onnx ...
    ```

- Manual grouping:

    ```bash
    atc --h2d_overlapped_with_compute=44,64,68 --model=model.onnx ...
    ```

## Dependency Constraints

- Currently takes effect only for static shape models.
- Currently supports only single-stream graphs.
- The feature takes effect only when the input at execution is Host memory. Currently limited by the ACL model execution API (the input is fixed in Device memory handling), it cannot be enabled end-to-end and is a reserved function. It can be used normally after the ACL API exposure of Host input.
- The manually configured boundary must be a non-negative integer and in ascending permutation.
- The manually configured boundary must be based on the input sorting and grouping result printed in the current model log.
- After the model structure or topology sorting changes, the group boundary needs to be reconfirmed.
