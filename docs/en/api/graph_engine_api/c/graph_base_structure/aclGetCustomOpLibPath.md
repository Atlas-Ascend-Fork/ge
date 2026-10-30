# aclGetCustomOpLibPath

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-27T16:29:26.668Z pushedAt=2026-10-28T13:31:15.404Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File

\#include <register/register\_base.h\>

## Feature Description

Obtains the path of the custom operator library.

This API is called by the internal framework and is referred to as an **internal API**. You do not need to call internal APIs directly or pay attention to them.

## Prototype

```c
const char *aclGetCustomOpLibPath()
```

## Parameters

None

## Return Value

Returns the path of the custom operator library. Multiple paths are separated by colons and are already arranged in the order of operator priority set by the user, with higher-priority paths placed earlier.

For details about setting the priority of custom operators, see the "Programming Guide \> Appendix \> Engineering Operator Development \> Operator Dynamic Library Compilation" section in *[Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md)*.

## Constraints

None

## Examples

```c
const char* path = aclGetCustomOpLibPath();
```
