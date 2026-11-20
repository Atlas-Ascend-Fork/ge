# dataflow.finalize

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:11:12.540Z pushedAt=2026-11-16T07:49:12.774Z nl=2 -->

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
# Initialize.
df.init(...)
# dataflow processing logic
# Release resources.
df.finalize()
```

## Constraints

None
