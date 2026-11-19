# get\_attr\_int\_list\_list

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:34:38.635Z pushedAt=2026-11-16T08:00:53.014Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the attribute value of the int two-dimensional array type with the specified name.

## Prototype

```python
get_attr_int_list_list(self, name: str) -> Tuple[int, List[List[int]]]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |

## Return Value

Obtains the return code and the attribute value of the int two-dimensional array type.

- If the attribute exists, the first element in the returned Tuple is FLOW\_FUNC\_SUCCESS, and the second element is the attribute value of the int two-dimensional array type.
- If the attribute does not exist, the tuple contains only the error code.

## Exception Handling

None

## Constraints

None
