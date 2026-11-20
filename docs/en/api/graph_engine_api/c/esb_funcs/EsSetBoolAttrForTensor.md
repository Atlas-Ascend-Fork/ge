# EsSetBoolAttrForTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:41:37.627Z pushedAt=2026-11-18T06:41:40.266Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Sets the boolean attribute of a tensor.

## Prototype

```c
uint32_t EsSetBoolAttrForTensor(EsCTensorHolder *tensor, const char *attr_name, bool value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |
| attr_name | Input | Attribute name. |
| value | Input | Attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a non-zero value. |

## Constraints

None
