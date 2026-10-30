# set\_output

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:15:57.083Z pushedAt=2026-10-29T03:22:42.005Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the tensor of the output with the specified index.

## Prototype

```python
set_output(self, index, output: Union[FlowMsg, np.ndarray, fw.FlowMsg], balance_config: BalanceConfig = None) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Specifies the index of the output. |
| output | Input | Specifies the Msg of the output. |
| balance_config | Input | Optional. Configuration related to balanced distribution of the output. |

## Return Value

- 0: SUCCESS
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None

## Constraints

None
