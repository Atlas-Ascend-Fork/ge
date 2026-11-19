# GetAllOpName

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:16:59.581Z pushedAt=2026-10-28T07:40:22.742Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains the name list of all cached operators registered in the **Graph**.

## Prototype

>[!NOTE]
>The API with the string data type will be deprecated in later versions. You are advised to use the API with a non-string data type.

```c++
graphStatus GetAllOpName(std::vector<std::string> &op_name) const
graphStatus GetAllOpName(std::vector<AscendString> &names) const

```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_name | Output | Names of all operators in the **Graph**. |
| names | Output | Names of all operators in the **Graph**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **SUCCESS**: The operation is successful.<br>**FAILED**: The operation fails. |

## Constraints

This API is an optional API and is used together with [AddOp](AddOp.md).
