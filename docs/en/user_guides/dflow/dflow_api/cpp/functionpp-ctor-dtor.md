# FunctionPp Constructor and Destructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:30:56.644Z pushedAt=2026-11-16T06:11:08.773Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Constructor and destructor of FunctionPp. The constructor returns a FunctionPp object.

## Prototype

```cpp
FunctionPp(const char_t *pp_name)
~FunctionPp() override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pp_name | Input | Name of the FunctionPp, which must be globally unique. |

## Return Value

Returns a FunctionPp object.

## Exception Handling

None.

## Constraints

None.
