# dataflow.finalize

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:07:34.177Z pushedAt=2026-10-29T03:01:21.906Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Releases the resources initialized by dataflow.

## Prototype

```python
finalize()
```

## Parameters

None

## Return Value

None

## Examples

```python
import dataflow as df
# Initialize
df.init(...)
# dataflow processing logic
# Release resources
df.finalize()
```

## Constraints

None
