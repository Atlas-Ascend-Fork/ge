# Overview

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:34:05.333Z pushedAt=2026-10-16T10:53:07.318Z -->

## Applicable Products

Supported on all chips.

## Description

`CompilePlatformInfo` is a read-only view of the platform information for the current schema-bound `compile` callback. It is returned by `get_compile_platform_info()` and cannot be directly constructed by users.

This object is valid only within the current `compile` callback. After the callback returns or raises an exception, calling any method throws a `RuntimeError`.

## Prototype

None

## Parameters

None

## Constraints

This object can only be used within the current synchronous `compile` callback.

## Examples

None
