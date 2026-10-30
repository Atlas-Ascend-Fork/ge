# --enable\_small\_channel

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-22T03:03:04.734Z pushedAt=2026-10-23T13:57:23.954Z -->

## Product Support

Supported on all chips.

## Feature Description

Enables or disables small channel optimization. When enabled, performance gains are achieved on convolutional layers with channel <= 4. It is recommended that you enable this switch in inference scenarios.

## Related Parameters

- After this parameter is configured, it is recommended to use it together with the [--insert\_op\_conf](--insert_op_conf.md) parameter (AIPP feature) to achieve better performance. When used together, due to software constraints, it can only be used with static AIPP, not with dynamic AIPP.

## Parameter Values

**Parameter values:**

- 0: (default) Disabled. Small channel optimization is disabled during model inference.
- 1: Enabled. Small channel optimization is enabled during model inference.

**Parameter value constraints:**
If the channel of the model input is less than or equal to 4, you are advised to enable this parameter and use it together with static AIPP ([--insert\_op\_conf](--insert_op_conf.md)) to achieve better performance. If performance degrades after enabling this parameter, you are advised to perform Tiling tuning.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --enable_small_channel=1 ...
```

## Usage Constraints

After this parameter is enabled, it is recommended to use it together with the AIPP feature [--insert\_op\_conf](--insert_op_conf.md); otherwise, there may be no performance gain.
