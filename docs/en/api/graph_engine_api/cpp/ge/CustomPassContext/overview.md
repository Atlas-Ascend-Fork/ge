# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:27:01.901Z pushedAt=2026-10-28T07:40:22.306Z nl=2 -->

Obtains the custom pass context object.

## Header File to Include

```c++
#include <register/register_custom_pass.h>
```

## Public Member Functions

```c++
CustomPassContext()
virtual ~CustomPassContext() = default
void SetErrorMessage(const AscendString &error_message)
graphStatus GetOptionValue(const AscendString &option_key, AscendString &option_value) const
void SetPassName(const AscendString &pass_name)
AscendString GetPassName() const
```
