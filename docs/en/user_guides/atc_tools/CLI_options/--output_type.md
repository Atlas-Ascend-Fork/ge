# --output\_type

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:34:29.752Z pushedAt=2026-10-27T01:46:08.630Z -->

## Product Support

All chips are supported.

## Function Description

Specifies the network output data type or the output type of a specified output node.

## Related Parameters

If the output type of a specified output node is set, it must be used together with the [--out\_nodes](--out_nodes.md) parameter.

## Parameter Values

**Parameter values:**

- FP32: Recommended for classification networks and detection networks.
- FP16: Recommended for classification networks and detection networks. It is usually used in scenarios where the output of one network serves as the input of another network.
- UINT8: Recommended for image super-resolution networks, which delivers better inference performance.
- INT8
- UINT16
- INT16
- UINT32
- INT32
- UINT64
- INT64
- DOUBLE
<!-- npu="950" id1 -->
- HIF8: Only the Ascend 950PR&950DT products support this type.
- FP8E5M2: Only the Ascend 950PR&950DT products support this type.
- FP8E4M3FN: Only the Ascend 950PR&950DT products support this type.
<!-- end id1 -->

**Parameter value constraints:**

After model conversion is complete, the preceding data types correspond to the following types in the generated OM offline model file:

- DT\_FLOAT
- DT\_FLOAT16
- DT\_UINT8
- DT\_INT8
- DT\_UINT16
- DT\_INT16
- DT\_UINT32
- DT\_INT32
- DT\_UINT64
- DT\_INT64
- DT\_DOUBLE
<!-- npu="950" id2 -->
- DT\_HIFLOAT8
- DT\_FLOAT8\_E5M2
- DT\_FLOAT8\_E4M3FN
<!-- end id2 -->

If the specific output data type of the network is not specified during model conversion, the operator data type of the last layer output of the original network model is used. If a type is specified, the type specified by this parameter is used, and the type specified by the [--is\_output\_adjust\_hw\_layout](--is_output_adjust_hw_layout.md) parameter does not take effect.

## Recommended Configuration and Benefits

None.

## Example

- Specify the network output data type

    ```bash
    atc --output_type=FP32 ...
    ```

- Specify the output data type of a specific output node

    For example: --output\_type="node1:0:FP16;node2:0:FP32", which means that the first output of node1 is set to FP16 and the first output of node2 is set to FP32. The specified nodes must be enclosed in double quotation marks, and multiple nodes must be separated by semicolons.

    In this scenario, this parameter must be used together with the [--out\_nodes](--out_nodes.md) parameter.

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow.pb --framework=3 --output=$HOME/module/out/tf_resnet50  --soc_version=<soc_version>  --output_type="conv1:0:FP16"  --out_nodes="conv1:0" ...
    ```

## Dependency Constraints

None.
