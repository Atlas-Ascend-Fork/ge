# aclGetCustomOpLibPath

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-11-17T15:46:22.489Z pushedAt=2026-11-18T06:42:59.782Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File

\#include <register/register\_base.h\>

## Function Description

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

For details about setting the priority of custom operators, see the "Programming Guide \> Appendix \> Engineering Operator Development \> Operator Dynamic Library Compilation" section in *[Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/master/docs/zh/guide/index.md)*.

## Constraints

None

## Examples

```c
const char* path = aclGetCustomOpLibPath();
```
