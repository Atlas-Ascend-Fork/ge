# Init (MetaMultiFunc Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:17:46.199Z pushedAt=2026-11-16T04:52:50.460Z nl=2 -->

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
