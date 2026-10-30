# CreateVector

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:05:30.849Z pushedAt=2026-10-28T07:40:22.461Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a vector constant.

## Prototype

- Creates a vector constant of the int64 type.

    ```c++
    EsTensorHolder CreateVector(const std::vector<int64_t> &value)
    ```

- Creates a vector constant of the int32 type.

    ```c++
    EsTensorHolder CreateVector(const std::vector<int32_t> &value)
    ```

- Creates a vector constant of the uint64 type.

    ```c++
    EsTensorHolder CreateVector(const std::vector<uint64_t> &value)
    ```

- Creates a vector constant of the uint32 type.

    ```c++
    EsTensorHolder CreateVector(const std::vector<uint32_t> &value)
    ```

- Creates a vector constant of the float type.

    ```c++
    EsTensorHolder CreateVector(const std::vector<float> &value)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Data vector of the vector constant. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder | Returns the created vector tensor holder; on failure, returns an invalid **EsTensorHolder**. |

## Constraints

None

## Examples

```c++
EsGraphBuilder builder("test_graph");
std::vector<int64_t> vec64 = {1, 2, 3};
auto v1 = builder.CreateVector(vec64);
```
