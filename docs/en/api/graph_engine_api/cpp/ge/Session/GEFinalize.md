# GEFinalize

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-26T13:14:25.274Z pushedAt=2026-10-28T07:42:19.796Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Exits GE and releases GE-related resources.

Within this API, a 2000 ms delay is added by default (the actual maximum delay can reach 2000 ms) for Device service log reporting, ensuring that ERROR-level and EVENT-level logs are not lost. You can set the **ASCEND\_LOG\_DEVICE\_FLUSH\_TIMEOUT** environment variable to 0 (command example: **export ASCEND\_LOG\_DEVICE\_FLUSH\_TIMEOUT=0**) to remove this default delay.

For a detailed description of the **ASCEND\_LOG\_DEVICE\_FLUSH\_TIMEOUT** environment variable, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## Prototype

```c++
Status GEFinalize()
```

## Parameters

None

## Return Value

None

## Constraints

None
