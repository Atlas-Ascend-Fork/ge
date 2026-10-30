# Init (MetaMultiFunc Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:56:43.099Z pushedAt=2026-10-29T01:54:28.858Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Initialization function of a user-defined flow func.

## Prototype

```cpp
int32_t Init(const std::shared_ptr<MetaParams> &params)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| MetaParams | Input | Parameter information used by the multi-func processing function. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
