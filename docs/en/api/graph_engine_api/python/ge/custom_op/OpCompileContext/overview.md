# Overview

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:39:00.875Z pushedAt=2026-10-16T10:53:07.337Z -->

## Applicable Products

Supported on all chips.

## Description

`OpCompileContext` is the read-only compilation context of the current schema-bound `compile` callback. It is returned by [get_compile_ctx](../get_compile_ctx.md) and cannot be directly constructed by users.

This object provides compilation option queries. It is a borrowed view that is valid only within the current callback; after the callback returns or throws an exception, calling any method raises `RuntimeError`. For platform resources, core count, and SoC information, use [get_compile_platform_info()](../get_compile_platform_info.md).

`OpCompileContext` only queries the compilation environment and does not store the user's compilation results. Users can temporarily store results in the implementation instance, but GE does not serialize, restore, or roll back these Python states.

## Prototype

None

## Parameters

None

## Constraints

This object can only be used within the current synchronous `compile` callback.

## Examples

None
