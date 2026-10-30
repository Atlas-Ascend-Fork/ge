# dataflow.FlowData

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:09:09.487Z pushedAt=2026-10-29T03:01:30.237Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A data node in a DataFlow Graph. Each FlowData corresponds to one input.

## Prototype

```python
FlowData(data_cls=Tensor, schema:Optional[TensorDesc]=None, name=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| data_cls | class | Currently only the default value Tensor is supported, indicating that FlowData receives data of the Tensor type. |
| schema | Optional[TensorDesc] | Description of data_cls. Since data_cls currently supports only Tensor, the value of schema is TensorDesc. |
| name | str | Node name. The framework automatically ensures name uniqueness. If not set, names such as FlowData, FlowData_1, FlowData_2, ... are automatically generated. |

## Return Value

Returns None in normal scenarios.

Returns raise DfException, indicating that the parameter type is incorrect. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
# Create an input node without specifying the data type and name. If the user does not specify name, the framework assigns a unique node name: FlowData FlowData_1
data = df.FlowData()
# Create an input node that accepts int32 data with the shape [1]
data = df.FlowData(schema=df.TensorDesc(df.DT_INT32, [1]))
# Create an input node without specifying the data type but with the name data0
data = df.FlowData(name="data0")
# Create an input node with the data type int32, shape [1], and name data0
data = df.FlowData(schema=df.TensorDesc(df.DT_INT32, [1]), name="data0")
```

## Constraints

None
