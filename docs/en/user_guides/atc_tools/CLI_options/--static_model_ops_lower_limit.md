# --static\_model\_ops\_lower\_limit

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T14:39:12.960Z pushedAt=2026-11-17T12:20:43.039Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

**In dynamic shape splitting, sets the minimum operator count threshold for static subgraphs.**

## Related Parameters

When [--mode](--mode.md) is set to 7, this parameter cannot be configured.

## Parameter Values

**Parameter value:**
An integer in the range of \[-1, INT64\_MAX\]. Any other value causes an error.

- When the value is \>=0:
  - If the number of operators is less than the threshold, the static subgraph is not split separately and is merged into the dynamic graph for dynamic execution.
  - If the number of operators is \>= the threshold, the original process remains unchanged (the static subgraph is still split).

- When the value is -1: all operators are executed in the dynamic graph.

For example, assume that the number of operators in a static subgraph is 4 and the threshold configured through this parameter is 10. In this case, the static subgraph is not split separately and is executed in the dynamic graph.

**Default value**: 4

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --static_model_ops_lower_limit=4 --model=model.onnx ...
```

## Dependency Constraints

None.
