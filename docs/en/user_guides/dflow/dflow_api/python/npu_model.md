# npu\_model

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:05:41.475Z pushedAt=2026-10-29T03:17:18.012Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

If a UDF is deployed on the host side, data needs to be copied from the device to the local memory for computation during execution. In the PyTorch scenario, if all computation is performed on the device side and the input and output are also on the device side, data needs to be copied from the device to the host during execution, and PyTorch then moves the data back to the device after execution, which affects execution performance. Using npu\_model can optimize this by triggering execution in a way that does not move data (that is, directly offloading execution to the device).

## Prototype

```python
Decorator @npu_model
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| optimize_level | int | - 1: In the PyTorch scenario, the UDF nn engine offloads input and output data to the device for execution. The default value is 1.<br>  - 2: Compiles the PyTorch model into a graph and directly exports it as an nn model, optimizing it for loading and execution as an npu model. This option must be used together with input_descs. Note: This configuration takes effect when decorating a class and cannot be configured when decorating a function. |
| input_descs | [[TensorDesc](dataflow-TensorDesc.md)] | When optimize_level=2, describes the input tensor of the graph exported from torch. Example:<br>input_descs=[TensorDesc(dtype = df.DT_INT64, shape = [2,1,4]),TensorDesc(dtype =<br>df.DT_FLOAT, shape = [2,1,4])],<br>If a dimension in shape is negative, the input is dynamic, and npu_model ultimately exports a dynamic graph. |
| num_returns | int | When decorating a function, indicates the number of outputs of the function. If this parameter is not set, the function returns one return value by default. You can choose either this parameter or the type annotations approach to specify the number and types of function return values. |
| resources | dict | Specifies the resource information required by the current func. Supports memory, num_cpus, and num_npus. The unit of memory is M. num_npus indicates the number of npu resources required. It is a reserved parameter and currently supports only 1. Example: {"memory": 100, "num_cpus": 1, "num_npus": 1} |
| env_hook_func | function | This hook function allows users to extend the necessary Python environment preparation or import operations before Python UDF initialization. |
| visible_device_enable | bool | When enabled, the UDF process automatically sets ASCEND_RT_VISIBLE_DEVICES based on the num_npus resources configured by the user, and calls the get_running_device_id API to obtain the corresponding logical ID. Currently num_npus supports only 1, so get_running_device_id returns 0 in this scenario. |

## Return Value

In normal scenarios, returns the decorated function.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
@df.npu_model(optimize_level=1)
class FakeModel1(nn.Module):
    def __init__(self):
        super().__init__()

    # Simulate model inference.
    @df.method()
    def forward(self, input_image):
        return F.interpolate(input_image, size=(256, 256), mode='bilinear')

@df.npu_model(optimize_level=1, input_descs=[df.TensorDesc(dtype=df.DT_FLOAT, shape=[1, 3, 768, 768])])
class FakeModel2(nn.Module):
    def __init__(self):
        super().__init__()
        self.mean = 0.5
        self.std = 0.5

    # Simulate model inference.
    @df.method()
    def forward(self, input_image):
        return (input_image - self.mean) / self.std

@df.npu_model()
def preprocess(input_image):
    # Simulate image cropping.
    transform = transforms.Compose([transforms.CenterCrop(512)])
    return transform(input_image)

@df.npu_model()
def postprocess(input_image):
    mean = 0.5
    std = 0.5
    img = input_image * std + mean
    return F.interpolate(img, size=(512, 512), mode='bilinear')
```

## Constraints

- The torch\_npu package corresponding to the Python version must be installed.
- The input and output must be npu tensors.
- One set of inputs corresponds to one set of outputs; streaming Input/Output is not supported.
