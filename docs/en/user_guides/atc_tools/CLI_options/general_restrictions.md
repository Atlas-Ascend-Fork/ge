# General Constraints

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-22T03:52:24.224Z pushedAt=2026-10-27T13:47:06.623Z -->

Before model conversion, be sure to check the following constraints:

- To convert a network model such as Faster RCNN into an offline model adapted to the AI processor, be sure to modify the prototxt model file first by referring to [Custom Network Modification (Caffe)](../custom_network_modify_caffe/README.md).

- Model conversion is supported for models whose original framework type is Caffe, TensorFlow, MindSpore, or ONNX:
  - When the original framework type is Caffe, MindSpore, or ONNX, the input data types are FP32, FP16, and UINT8 (implemented by configuring data preprocessing [--insert\_op\_conf](--insert_op_conf.md)).
  - When the original framework type is TensorFlow, the input data types are FP16, FP32, UINT8, INT32, INT64, and BOOL.

- For Caffe framework network models: the input data supports a maximum of four dimensions, and dimension transformation operators (Reshape, ExpandDims, etc.) cannot output five dimensions.

- For all layer operators in the model except the const operator, the input and output must satisfy dim!=0.

- Only the operators in the *[Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist)* > "Ascend IR Operator Specifications" are supported, and the corresponding operator constraints must be met.

- Due to software constraints (input data of DT\_INT8 is not supported in dynamic shape scenarios), when a quantized deployment model is converted using the ATC tool, dynamic shape-related parameters such as [--dynamic\_batch\_size](--dynamic_batch_size.md) and [--dynamic\_image\_size](--dynamic_image_size.md) cannot be used; otherwise, model conversion will fail.

- When a deployment model quantized by the AMCT tool is converted using the ATC tool, high-precision features can no longer be used. For example, **force\_fp32** or **must\_keep\_origin\_dtype (original image FP32 input) can no longer be configured through the [--precision\_mode](--precision_mode.md) parameter**; **origin** can no longer be configured through the [--precision\_mode\_v2](--precision_mode_v2.md) parameter; and the **high\_precision** parameter can no longer be configured through [--op\_precision\_mode](--op_precision_mode.md). Setting quantization parameters in high-precision mode yields neither the performance benefits of quantization nor the precision benefits of high-precision mode.
