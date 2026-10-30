# --optypelist\_for\_implmode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:32:13.183Z pushedAt=2026-10-26T13:20:44.165Z -->

## Function Description

**This parameter is no longer evolving and will be deprecated in later versions. Do not use it.**

Sets the implementation mode of the operators in the optype list. The operator implementation modes include high\_precision, high\_performance.

## Related Parameters

- This parameter must be used together with the [--op\_select\_implmode](--op_select_implmode.md) parameter. All operators set through the [--optypelist\_for\_implmode](--optypelist_for_implmode.md) parameter uniformly use the implementation mode specified by the [--op\_select\_implmode](--op_select_implmode.md) parameter. Different implementation modes cannot be set for individual operators in the list.
- When used together with the [--op\_select\_implmode](--op_select_implmode.md) parameter, this parameter cannot be used simultaneously with the [--op\_precision\_mode](--op_precision_mode.md) parameter. If both are configured, only the mode specified by the [--op\_precision\_mode](--op_precision_mode.md) parameter takes effect. For the combined running process of the preceding parameters, see [Figure 1](--op_precision_mode.md#fig1).

## Parameter Values

**Parameter Value**: operator list.

**Parameter Value Constraints:**

- The OpType of the operators in this list must be the OpType of operators defined based on Ascend IR. For details about how to determine the correspondence between operators in the original framework network model and operators supported by the AI processor, see [How Do I Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by the AI Processor](../FAQ/operator_correspondence_guide.md).
- The operators in this list use the implementation mode specified by the [--op\_select\_implmode](--op_select_implmode.md) parameter, and only high\_precision and high\_performance are supported. Multiple operators are separated by commas (,).
- This parameter takes effect only on the specified operators. Operators that are not specified use the default implementation mode.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --op_select_implmode=high_precision  --optypelist_for_implmode=Pooling,SoftmaxV2 ...
```

The preceding configuration example indicates that the unified high-precision mode is used for the Pooling and SoftmaxV2 operators, and that operators not specified use their default implementation mode.

## Dependencies and Constraints

None.
