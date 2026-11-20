# dataflow.Framework

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:15:46.304Z pushedAt=2026-11-16T07:51:15.035Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the framework type of the original network model.

## Prototype

Not involved

## Parameters

The enumeration values are as follows:

- Framework.TENSORFLOW
- Framework.ONNX
- Framework.MINDSPORE

## Return Value

None

## Examples

```python
import dataflow as df
framework = df.Framework.TENSORFLOW
pp1 = df.GraphProcessPoint(framework,...)
```

## Constraints

None
