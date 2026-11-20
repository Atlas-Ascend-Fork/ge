# set\_output

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:33:46.907Z pushedAt=2026-11-17T03:39:12.046Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the tensor of the output at the specified index.

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
