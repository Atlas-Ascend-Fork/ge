# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:36:53.974Z pushedAt=2026-10-28T07:42:19.733Z nl=2 -->

The APIs provided by the OpLibRegister class are internal APIs. They currently provide registration capability for custom operator dynamic libraries, allowing the custom operator project framework to register specified initialization functions. You do not need to be concerned with these APIs.

## Header File to Include

```c++
#include <register/op_lib_register.h>
```

## Public Member Functions

```c++
explicit OpLibRegister(const char_t *vendor_name)
OpLibRegister(OpLibRegister &&other) noexcept
OpLibRegister(const OpLibRegister &other)
OpLibRegister &operator=(const OpLibRegister &) = delete
OpLibRegister &operator=(OpLibRegister &&) = delete
~OpLibRegister()
OpLibRegister &RegOpLibInit(OpLibInitFunc func)
```
