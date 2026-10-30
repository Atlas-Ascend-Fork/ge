# get\_attr\_str\_list

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:49:44.967Z pushedAt=2026-10-29T03:11:21.043Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the attribute value of the string array type with a specified name.

## Prototype

```python
get_attr_str_list(self, name: str) -> Tuple[int, List[str]]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |

## Return Value

Obtains the return code and the attribute value of the string array type.

- If the attribute exists, the first element in the returned tuple is FLOW\_FUNC\_SUCCESS, and the second element is the attribute value of the string array type.
- If the attribute does not exist, the tuple contains only one element, which is the error code.

## Exception Handling

None

## Constraints

None
