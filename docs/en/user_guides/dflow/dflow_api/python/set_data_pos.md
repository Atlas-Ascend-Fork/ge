# set\_data\_pos

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:12:40.140Z pushedAt=2026-10-29T03:21:51.422Z nl=2 -->

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
