# GetResourceContext

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:31:12.609Z pushedAt=2026-10-28T07:40:22.800Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Obtains the corresponding resource context object by the resource identifier **key**.

## Prototype

```c++
ResourceContext *GetResourceContext(const ge::AscendString &key)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Unique identifier of the resource, specified by the infershape function of the resource operator. |

## Return Value

Resource context object.

The basic definition is as follows. Resource operators can be extended based on it.

```c++
struct ResourceContext {  virtual ~ResourceContext() {}};
```

Used to store resource-related information, such as shape and datatype.

## Constraints

If no resource context manager pointer is passed when creating the InferenceContext using the [Create](Create.md) API, this API returns a null pointer. Therefore, check for null before using its return value.
