# InferShapeAndType

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-26T12:25:19.150Z pushedAt=2026-10-28T07:42:19.691Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Infers the shape and DataType of the Operator output.

For the definition of the DataType data type, see [DataType](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

## Prototype

```c++
graphStatus InferShapeAndType()
```

## Parameters

None

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS on success; otherwise, returns GRAPH\_FAILED.

## Constraints

None
