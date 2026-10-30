# --is\_output\_adjust\_hw\_layout

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:13:47.628Z pushedAt=2026-10-26T02:50:13.197Z -->

## Product Support

All chips supported

## Function Description

Used together with the [--out\_nodes](--out_nodes.md) parameter to specify that the output data type of the network model is float16 and the data format is NC1HWC0. Configuring this parameter alone is ineffective, but the model conversion still succeeds.

## Related Parameters

- When --is\_output\_adjust\_hw\_layout is set to true, it must be used together with [--out\_nodes](--out_nodes.md). The nodes specified by --out\_nodes output data of the float16 type in the NC1HWC0 format.

    The number of values of --is\_output\_adjust\_hw\_layout must match the number of nodes specified by --out\_nodes. Separate multiple values with commas (,).

- When [--framework](--framework.md) is set to 1 and the network model is a MindSpore model, setting this parameter does not take effect, but the model conversion succeeds.

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
