# RegOpLibInit

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:36:58.488Z pushedAt=2026-10-28T07:42:19.736Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/op\_lib\_register.h\>
- Library file: libregister.so

## Description

Registers the initialization function of a custom operator dynamic library.

## Prototype

```c++
OpLibRegister &RegOpLibInit(OpLibInitFunc func)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| func | Input | Custom initialization function to be registered, of the type OpLibInitFunc.<br>using OpLibInitFunc = uint32_t (*)(ge::AscendString&); |

## Return Value

Returns an **OpLibRegister** object that newly registers the **OpLibInitFunc** function func.

## Constraints

None

## Examples

```c++
uint32_t Init(ge::AscendString&) {
  // init func
  return 0;
}

REGISTER_OP_LIB(vendor_1).RegOpLibInit(Init); // Register the initialization function Init for the vendor named vendor_1.
```
