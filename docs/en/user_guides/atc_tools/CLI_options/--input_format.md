# --input\_format

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:10:27.759Z pushedAt=2026-10-26T01:26:34.924Z -->

## Product Support

All chips supported

## Function Description

Specifies the format of the model input data.

## Related Parameters

None.

## Parameter Values

**Parameter values:**

- When the original framework is Caffe, NCHW and ND (indicating that any dimension format is supported, with N <= 4) are supported, and the default value is NCHW.
- When the original framework is ONNX, NCHW, NCDHW, and ND (indicating that any dimension format is supported, with N <= 4) are supported, and the default value is NCHW.
- When the original framework is TensorFlow, five input formats are supported: NCHW, NHWC, ND, NCDHW, and NDHWC. The default value is NHWC.
  - If the TensorFlow model is output by an ONNX model conversion tool, this parameter is mandatory and its value must be NCHW.
  - If the original model contains operators with the data\_format input parameter, this parameter is mandatory. The recommended value is ND. During model conversion, the specific format is derived based on the operators with the data\_format attribute. If you cannot determine the input data format, it is recommended to set this parameter to ND.

- When the original framework is MindSpore, only NCHW can be configured. Setting other values takes no effect, but the model conversion still succeeds.

**Default values:**
The default value is NCHW for Caffe, MindSpore, and ONNX, and NHWC for TensorFlow.

**Constraints:**

- If AIPP is enabled during model conversion, during inference, the input image data must be in NHWC layout. In this scenario, the format of the input node that is ultimately connected to AIPP is forcibly changed to NHWC, which may be inconsistent with the format specified by the [--input\_format](--input_format.md) parameter in the atc model conversion command.
- If the [--insert\_op\_conf](--insert_op_conf.md) parameter is also configured, the [--input\_format](--input_format.md) parameter can only be set to NCHW or NHWC.
- If a model has multiple inputs, the different inputs must be set to the same data format.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --input_format=NCHW ...
```

## Dependencies and Constraints

None.
