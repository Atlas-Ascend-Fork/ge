# set\_multi\_outputs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:15:34.021Z pushedAt=2026-10-29T03:22:34.942Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the tensor of the output with the specified index in batches.

## Prototype

```python
set_multi_outputs(self, index, outputs: List[Union[FlowMsg, np.ndarray, fw.FlowMsg]], balance_config: BalanceConfig) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Specifies the index of the output. |
| output | Input | Specifies the Msg of the output. |
| balance_config | Input | Configuration related to balanced distribution of the output. |

## Return Value

- 0: SUCCESS
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None

## Constraints

None
