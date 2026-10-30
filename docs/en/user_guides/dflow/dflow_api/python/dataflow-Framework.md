# dataflow.Framework

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:12:57.827Z pushedAt=2026-10-29T03:03:31.933Z nl=2 -->

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
