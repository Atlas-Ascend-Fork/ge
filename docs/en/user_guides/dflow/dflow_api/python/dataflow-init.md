# dataflow.init

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:14:47.728Z pushedAt=2026-10-29T03:04:10.858Z nl=2 -->

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

Returns None in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
df.init(...)
```

## Constraints

None
