# ToStringInfo

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:46:30.337Z pushedAt=2026-10-28T07:40:22.859Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the information about the current logical stream.

## Prototype

```c++
AscendString ToStringInfo() const
```

## Parameters

None

## Return Value

Logical stream information of the **AscendString** type, for example:

```c++
logic_stream_id: 0, user_stream_label: 11, is_assigned_by_user_stream_pass: false, attached_stream_ids: , physical_model_stream_num: 1, hccl_followed_stream_num: 0.
```

The corresponding information is as follows:

- Logical stream ID: 0.
- User stream label: 11.
- Whether it comes from a user-registered stream allocation pass: false.
- Attached slave stream ID.
- Number of actual physical streams: 1.
- Number of communication secondary streams: 0.

## Constraints

None
