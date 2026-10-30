# run\_flow\_model

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:09:13.656Z pushedAt=2026-10-29T03:20:29.888Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Synchronously executes the specified model.

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
