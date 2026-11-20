# capture\_tensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:57:50.168Z pushedAt=2026-10-16T10:53:07.630Z -->

## Applicable Products

Supported on all chips.

## Description

Marks the **Tensor** to be captured in the pattern graph. Captured tensors are saved in call order and can be read later through match\_result.get\_captured\_tensor\(index\).

## Function Prototype

```python
capture_tensor(source: Union[TensorHolder, Node, NodeIo], index: Optional[int] = None) -> Pattern
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| source | Input | Source of the Tensor to be captured. Supports **TensorHolder**, **Node**, or **NodeIo**. |
| index | Input | Output index. When **source** is **TensorHolder** or **Node**, the output index can be specified, defaulting to 0 if not specified; when **source** is **NodeIo**, it does not need to be passed. |

## Return Value

| Type | Description |
| --- | --- |
| Pattern | Returns the current **Pattern**, supporting chained calls. |

## Constraints

None

## Example

```python
from ge.passes import create_pattern

# builder is the pattern graph constructor, and add and matmul are the nodes already built in the pattern graph.
pat = create_pattern(builder.build_and_reset([add]))
pat.capture_tensor(matmul)
pat.capture_tensor(add)
```
