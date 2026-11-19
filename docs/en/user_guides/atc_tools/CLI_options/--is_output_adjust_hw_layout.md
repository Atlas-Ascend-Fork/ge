# --is\_output\_adjust\_hw\_layout

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-16T14:07:09.153Z pushedAt=2026-11-17T10:57:50.771Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Used together with the [--out\_nodes](--out_nodes.md) parameter to specify that the output data type of the network model is float16 and the data format is NC1HWC0. Configuring this parameter alone is ineffective, but the model conversion still succeeds.

## Related Parameters

- When --is\_output\_adjust\_hw\_layout is set to true, it must be used together with [--out\_nodes](--out_nodes.md). The nodes specified by --out\_nodes output data of the float16 type in the NC1HWC0 format.

    The number of values of --is\_output\_adjust\_hw\_layout must match the number of nodes specified by --out\_nodes. Separate multiple values with commas (,).

- When [--framework](--framework.md) is set to 1 and the network model is a MindSpore framework model, setting this parameter is invalid, but the model conversion still succeeds.

## Parameter Values

- true: specifies that the output data type of the network model is float16 and the data format is NC1HWC0.
- false: (default) not specified.

## Recommended Configuration and Benefits

None.

## Example

- If --out\_nodes is configured with one node

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --is_output_adjust_hw_layout=true  --out_nodes="prob:0" --soc_version=<soc_version> ...
    ```

- If --out\_nodes is configured with multiple nodes

    ```bash
    atc --model=$HOME/module/resnet50_tensorflow.pb --framework=3 --output=$HOME/module/out/tf_resnet50 --is_output_adjust_hw_layout="true,true"  --out_nodes="prob:0;prob:1" --soc_version=<soc_version> ...
    ```

## Dependency Constraints

None.
