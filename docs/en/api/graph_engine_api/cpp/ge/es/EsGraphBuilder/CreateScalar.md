# CreateScalar

<!-- md-trans-meta sourceCommit=348a841c78ec999f62926be0bba26d6edb93fdf0 translatedAt=2026-11-17T02:59:04.043Z pushedAt=2026-11-18T02:41:08.916Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a scalar constant.

## Prototype

- Creates a scalar constant of the int64 type.

    ```c++
    EsTensorHolder CreateScalar(int64_t value)
    ```

- Creates a scalar constant of the int32 type.

    ```
    EsTensorHolder CreateScalar(int32_t value)
    ```

- Creates a scalar constant of the uint64 type.

    ```
    EsTensorHolder CreateScalar(uint64_t value)
    ```

- Creates a scalar constant of the uint32 type.

    ```
    EsTensorHolder CreateScalar(uint32_t value)
    ```

- Creates a scalar constant of the float type.

    ```
    EsTensorHolder CreateScalar(float value)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Scalar value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder | Returns the created scalar tensor holder; on failure, returns an invalid **EsTensorHolder**. |

## Constraints

None

## Examples

```c++
EsGraphBuilder builder("test_graph");
auto s1 = builder.CreateScalar(int64_t(42));
```
