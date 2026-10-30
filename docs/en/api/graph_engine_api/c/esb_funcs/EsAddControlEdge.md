# EsAddControlEdge

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-27T16:13:23.201Z pushedAt=2026-10-28T13:18:55.194Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Controls edge connection.

## Prototype

```c
uint32_t EsAddControlEdge(EsCTensorHolder *dest_ctrl_tensor, EsCTensorHolder **src_ctrl_tensors,int64_t ctrl_tensors_num)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dest_ctrl_tensor | Input | Control edge connection object. |
| src_ctrl_tensors | Input | Control edge input. |
| ctrl_tensors_num | Input | Number of control edges. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a failure value. |

## Constraints

None

## Examples

```c
auto tensor0 = _builder->CreateInput(0).GetCTensorHolder();
auto tensor1 = _builder->CreateInput(1).GetCTensorHolder();
auto tensor2 = _builder->CreateInput(2).GetCTensorHolder();
std::vector<EsCTensorHolder *> ctrl_ins = {tensor1, tensor2};
EsAddControlEdge(tensor0, ctrl_ins.data(), 2);
```
