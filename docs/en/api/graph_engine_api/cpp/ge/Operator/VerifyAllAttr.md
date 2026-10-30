# VerifyAllAttr

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:33:10.833Z pushedAt=2026-10-28T07:42:19.717Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Based on the value of disableCommonVerifier, verifies whether the attributes in the Operator are valid and whether the inputs and outputs of the Operator are valid.

## Prototype

```c++
graphStatus VerifyAllAttr(bool disable_common_verifier = false)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| disable_common_verifier | Input | When set to false, only the validity of attributes is verified. When set to true, the validity of all inputs and outputs of the Operator is additionally verified.<br>The default value is false. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS if the derivation succeeds; otherwise, returns GRAPH\_FAILED.

## Constraints

None
