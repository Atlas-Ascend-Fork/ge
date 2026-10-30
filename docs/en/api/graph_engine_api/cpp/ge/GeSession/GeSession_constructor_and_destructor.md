# GeSession Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:48:49.962Z pushedAt=2026-10-28T07:40:22.649Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

**GeSession** constructor and destructor.

## Prototype

```c++
explicit GeSession(const std::map<AscendString, AscendString> &options)
~GeSession()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| options | Input | A map table whose keys are parameter types and values are parameter values, both in string format, describing the configuration information of **GeSession**.<br>Generally, this parameter can be left empty, in which case it stays consistent with the global options passed to **GEInitializeV2**.<br>If you need to configure the current **GeSession** separately, you can configure it through this parameter. For supported configuration items, see the session-level parameters in [options parameter description](../options_params/options_parameters_description.md). |

## Return Value

None

## Constraints

**GeSession** does not support concurrent execution. In addition, **GeSession** exclusively occupies resources, so when multiple **GeSession** instances are created simultaneously, **GeSession** may fail to be created due to insufficient resources.
