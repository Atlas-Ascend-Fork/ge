# dataflow.init

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:18:12.101Z pushedAt=2026-11-16T07:51:48.108Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Options for initializing dataflow.

## Prototype

```python
init(flow_options)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| flow_options | Dict[str, str] | Options for initializing dataflow. The options currently support the global and session levels.<br>Adapt the configuration examples to the Python language. |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
df.init(...)
```

## Constraints

None
