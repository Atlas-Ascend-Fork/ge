# How to Convert an Operator Description File into an Offline Model

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-27T14:35:58.258Z pushedAt=2026-10-28T10:38:33.930Z nl=2 -->

This section provides the detailed steps for converting a single-operator description file into an offline model.

1. Construct a single-operator description file by referring to the parameter description in [What Is a Single-Operator Description File](singleop_desc_intro.md) and the [Configuration File Example](config_file_example.md). This section uses the Add single-operator add.json with the format ND as an example.
2. As the user who runs the CANN software package, upload the single-operator description file constructed in step 1 to any directory in the development environment, for example, the $HOME/singleop/ directory.
3. Execute the following command to generate an offline model. (The directories and files used in the following command are examples. Please use the actual ones.)

    ```bash
    atc --singleop=$HOME/singleop/add.json --output=$HOME/singleop/out/op_model --soc_version=<soc_version>
    ```

    - --singleop: specifies the _add.json_ single-operator description file.
    - --output: path for storing the converted offline model.
    - --soc\_version: model of the AI processor.

    For details about the parameters, see [Parameter Description](../CLI_options/README.md).

4. If the following information is displayed, the model conversion is successful.

    <!-- npu="950,A3,910b,910,310p,310b" id1 -->
    If the model conversion fails, see *[Fault Handling](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/troubleshooting/00_troubleshooting.md)*\> "Error Code Reference" for auxiliary location.
    <!-- end id1 -->

    ```textconsole
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the offline model file \*.om in the path specified by the output parameter.
