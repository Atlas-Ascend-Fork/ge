# --is\_input\_adjust\_hw\_layout

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:12:34.275Z pushedAt=2026-10-26T02:49:46.784Z -->

## Product Support

Supported on all chips

## Function Description

Used together with the [--input\_fp16\_nodes](--input_fp16_nodes.md) parameter to specify that the input data type of the network model is float16 and the data format is NC1HWC0. Configuring this parameter alone is invalid, but the model conversion succeeds.

## Related Parameters

- When --is\_input\_adjust\_hw\_layout is set to true, it must be used together with [--input\_fp16\_nodes](--input_fp16_nodes.md). The nodes specified by --input\_fp16\_nodes use float16 as the input data type and NC1HWC0 as the input data format.

    The number of values of --is\_input\_adjust\_hw\_layout must match the number of nodes specified by --input\_fp16\_nodes. Separate multiple values with commas (,).

- When [--framework](--framework.md) is set to 1 and the network model is a MindSpore model, setting this parameter does not take effect, but the model conversion succeeds.

## Parameter Values

- true: specifies that the input data type of the network model is float16 and the data format is NC1HWC0.
- false: (default value) does not specify.

## Recommended Configuration and Benefits

None.

## Example

- If --input\_fp16\_nodes is configured with one node:

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --is_input_adjust_hw_layout=true  --input_fp16_nodes="data" --soc_version=<soc_version> ...
    ```

- If --input\_fp16\_nodes is configured with multiple nodes:

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --is_input_adjust_hw_layout="true,true"  --input_fp16_nodes="data1;data2" --soc_version=<soc_version> ...
    ```

## Dependency Constraints

None.
