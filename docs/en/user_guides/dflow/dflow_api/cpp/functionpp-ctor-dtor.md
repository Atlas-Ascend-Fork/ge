# FunctionPp Constructor and Destructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:01:49.416Z pushedAt=2026-10-28T12:30:42.545Z nl=2 -->

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
