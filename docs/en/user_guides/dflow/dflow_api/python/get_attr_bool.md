# get\_attr\_bool

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:32:31.122Z pushedAt=2026-11-16T08:00:40.695Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the value of the bool-type attribute with the specified name.

## Prototype

```python
get_attr_bool(self, name: str) -> Tuple[int, bool]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |

## Return Value

Obtains the return code and the bool-type attribute value.

- If the attribute exists, the first element in the returned Tuple is FLOW\_FUNC\_SUCCESS, and the second element is the bool-type attribute value.
- If the attribute does not exist, the tuple contains only the error code.

## Exception Handling

None

## Constraints

None
