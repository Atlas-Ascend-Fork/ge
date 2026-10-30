# get\_attr\_int\_list

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:48:51.586Z pushedAt=2026-10-29T03:11:15.422Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the value of the int array attribute with the specified name.

## Prototype

```python
get_attr_int_list(self, name: str) -> Tuple[int, List[int]]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |

## Return Value

Obtains the return code and the value of the int array attribute.

- If the attribute exists, the first element in the returned tuple is FLOW\_FUNC\_SUCCESS, and the second element is the value of the int array attribute.
- If the attribute does not exist, the tuple contains only the error code.

## Exception Handling

None

## Constraints

None
