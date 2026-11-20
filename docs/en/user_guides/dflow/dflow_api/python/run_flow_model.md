# run\_flow\_model

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:21:21.815Z pushedAt=2026-11-17T03:39:12.023Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Synchronously execute the specified model.

## Prototype

```python
run_flow_model(self, model_key:str, input_msgs: List[FlowMsg], timeout: int) -> Tuple[int, FlowMsg]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model_key | Input | Key of the specified model. When DataFlow is developed in C++, this key is the same as the name specified in AddInvokedClosure. |
| input_msgs | Input | Input provided to the model. |
| timeout | Input | Timeout for waiting for model execution, in ms. -1 indicates no timeout. |

## Return Value

- 0: SUCCESS
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None

## Constraints

None
