# --framework

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:05:16.051Z pushedAt=2026-10-27T09:01:52.377Z -->

## Product Support

All chips are supported.

## Function Description

Type of the original network model framework.

<!-- npu="950" id1 -->
The Ascend 950PR&950DT products **do not support the Caffe** framework: the Caffe framework is no longer evolving in this product form, and its functionality is not guaranteed.
<!-- end id1 -->

<!-- npu="A3" id2 -->
The Atlas A3 products **do not support the Caffe** framework: the Caffe framework is no longer evolving in this product form, and its functionality is not guaranteed.
<!-- end id2 -->

<!-- npu="910b" id3 -->
The Atlas A2 products **do not support the Caffe** framework: the Caffe framework is no longer evolving in this product form, and its functionality is not guaranteed.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--framework_res.md#id1 -->

## Related Parameters

None.

## Parameter Values

**Parameter values:**

- 0: Caffe
- 1: MindSpore framework \*.air format model file or a standard \*.air format file exported by TorchAir through export
- 3: TensorFlow
- 5: ONNX

**Parameter value constraints:**

- When [--mode](--mode.md) is 1, this parameter is optional. You can specify Caffe, TensorFlow, or ONNX original models to be converted into JSON files. If it is not specified, an offline model is converted into a JSON file by default. If it is specified, ensure that the [--om](--om.md) model and the [--framework](--framework.md) type correspond to each other. For example:

    ```bash
    --mode=1 --framework=0 --om=$HOME/module/resnet50.prototxt
    --mode=1 --framework=3 --om=$HOME/module/resnet50_tensorflow.pb
    --mode=1 --framework=5 --om=$HOME/module/resnet50.onnx
    ```

- When [--mode](--mode.md) is 0 or 3, this parameter is mandatory. You can specify Caffe, TensorFlow, MindSpore, or ONNX.
- When the value is 0, that is, a Caffe framework network model, the model includes a model file with the .prototxt suffix and a weight file with the .caffemodel suffix. The op name and op type of these two files must be consistent (including case).
- When the value is 3, that is, a TensorFlow framework network model, only the FrozenGraphDef format is supported, that is, a model file with the .pb suffix. The pb file is stored in protobuf format, and both the network model and weight data are stored in the same file.
- When the value is 5, that is, an ONNX-format network model, operators of opset v11\~v18 in the ai.onnx operator domain are supported. A pth model of the PyTorch framework can be converted to an ONNX-format model or exported as a standard \*.air format file through TorchAir before model conversion.
- When the value is 1 and the model is a MindSpore framework network model, be sure to note the following restrictions:
  - During model conversion, only model files with the \*.air suffix are supported.
  - [--mode](--mode.md) can only be set to 0.
  - [--input\_format](--input_format.md) can only be set to NCHW. Setting other values takes no effect, but model conversion still succeeds.
  - Under the MindSpore framework, the [--input\_shape](--input_shape.md), [--out\_nodes](--out_nodes.md), [--is\_output\_adjust\_hw\_layout](--is_output_adjust_hw_layout.md), [--input\_fp16\_nodes](--input_fp16_nodes.md), [--is\_input\_adjust\_hw\_layout](--is_input_adjust_hw_layout.md), and [--op\_name\_map](--op_name_map.md) parameters do not take effect, but model conversion still succeeds.
  - When the model size exceeds 2 GB, saving the model in the MindSpore framework also generates the \*.air file, the weight folder, and the weight files in it. During model conversion, the weight folder and the \*.air file must be placed in the same directory. Otherwise, model conversion reports an error.

## Recommended Configuration and Benefits

None.

## Example

- ONNX network model:

    ```bash
    atc --mode=0 --framework=5 --model=$HOME/module/resnet50.onnx --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>
    ```

- TensorFlow framework:

    ```bash
    atc --mode=0 --framework=3 --model=$HOME/module/resnet50_tensorflow.pb --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version>
    ```

- Caffe framework:

    ```bash
    atc --mode=0 --framework=0 --model=$HOME/module/resnet50.prototxt --weight=$HOME/module/resnet50.caffemodel  --output=$HOME/module/out/caffe_resnet50 --soc_version=<soc_version>
    ```

- \*.air format model file:

    ```bash
    atc --mode=0 --framework=1 --model=$HOME/module/ResNet50.air --output=$HOME/module/out/ResNet50_mindspore --soc_version=<soc_version>
    ```

## Constraints

- If you use Caffe framework network models such as Faster RCNN, YOLOv3, YOLOv2, and SSD for model conversion, note that these networks contain operator structures that are not defined in the original Caffe framework, such as ROIPooling, Normalize, PSROI Pooling, and Upsample. To enable the AI processor to support these networks, you need to extend the original Caffe framework network model, which reduces the workload of developing custom operators and post-processing code. For details about the extension method, see [Custom Network Modification (Caffe)](../custom_network_modify_caffe/README.md).
- For original TensorFlow framework network models, if control flow operators (such as Switch, Merge, LoopCond, Case, and While) exist, such network models cannot be directly converted using the ATC tool. You need to first convert the network model with control flow operators into a network model with function-type operators, and then use the ATC tool to convert it into an offline model adapted to the AI processor. For details about the conversion method, see [Custom Network Modification (TensorFlow)](../custom_network_modify_tf/README.md).
