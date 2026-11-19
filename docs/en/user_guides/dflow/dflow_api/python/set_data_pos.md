# set\_data\_pos

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:26:08.259Z pushedAt=2026-11-17T03:39:12.034Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the position of the output data in the weight matrix.

## Prototype

```python
set_data_pos(self, pos: List[Tuple[int, int]])
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pos | Input | Position of the output data in the weight matrix. For example, [(0, 0), (1, 0)] indicates that the two data items are located at positions (0, 0) and (1, 0) in the weight matrix, respectively. |

## Return Value

None

## Exception Handling

None

## Constraints

The position coordinates of pos must be within the rows and columns of the weight matrix.
