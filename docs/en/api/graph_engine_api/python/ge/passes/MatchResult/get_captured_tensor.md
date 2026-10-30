# get\_captured\_tensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:53:39.196Z pushedAt=2026-10-16T10:53:07.615Z -->

## Applicable Products

Supported on all chips.

## Description

Returns the **capture_index**-th captured Tensor in **capture_tensor** order.

## Prototype

```python
get_captured_tensor(capture_index: int) -> NodeIo
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **capture_index** | Input | Capture sequence number, which must be consistent with the call order of **capture_tensor**. |

## Return Value

| Type | Description |
| --- | --- |
| NodeIo | Returns the corresponding captured tensor, which contains the **node** and **index** attributes. |

## Constraints

**capture_index** must be consistent with the order of **capture_tensor**; otherwise, the C++ layer fails and throws an exception.

## Examples

```python
# match_result is the input parameter of replacement(self, match_result) or
# the input parameter of meet_requirements(self, match_result).
nodes = match_result.get_matched_nodes()
tensor0 = match_result.get_captured_tensor(0)  #NodeIo
n0, idx0 = tensor0.node, tensor0.index
```
