# get\_attr\_float

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:47:13.974Z pushedAt=2026-10-29T03:11:09.713Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the value of the float-type attribute with the specified name.

## Prototype

```python
get_attr_float(self, name: str) -> Tuple[int, float]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |

## Return Value

Obtains the return code and the float-type attribute value.

- If the attribute exists, the first element in the returned Tuple is FLOW\_FUNC\_SUCCESS, and the second element is the float-type attribute value.
- If the attribute does not exist, the Tuple contains only one element, which is the error code.

## Exception Handling

None

## Constraints

None
