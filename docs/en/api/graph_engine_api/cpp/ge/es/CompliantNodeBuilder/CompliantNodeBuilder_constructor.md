# CompliantNodeBuilder Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:31:26.355Z pushedAt=2026-10-28T07:40:22.326Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

**CompliantNodeBuilder** constructor and destructor.

## Prototype

```c++
explicit CompliantNodeBuilder(ge::Graph *graph)
~CompliantNodeBuilder()
CompliantNodeBuilder(CompliantNodeBuilder &&) noexcept
CompliantNodeBuilder(const CompliantNodeBuilder &)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph object to which it belongs. |

## Return Value

Returns a **CompliantNodeBuilder** object.

## Constraints

None
