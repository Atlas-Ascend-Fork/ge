# get\_attr\_int

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:49:05.977Z pushedAt=2026-10-29T03:11:18.253Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the int-type attribute value with the specified name.

## Prototype

```python
get_attr_int(self, name: str) -> Tuple[int, int]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |

## Return Value

Obtains the return code and the int-type attribute value.

- If the attribute exists, the first element in the returned Tuple is FLOW\_FUNC\_SUCCESS, and the second element is the int-type attribute value.
- If the attribute does not exist, the Tuple contains only the error code.

## Exception Handling

None

## Constraints

None
