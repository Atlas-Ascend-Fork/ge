# Converting a Model

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:17:45.968Z pushedAt=2026-10-28T08:13:04.295Z nl=2 -->

The following uses the Switch\_v1.pb network model as an example to demonstrate how to convert a network model with control flow operators into a function-type operator network model, and then convert it into an offline model adapted to the AI processor using the ATC tool. First, see [Obtaining the Switch\_v1.pb Network Model](./get_switch_v1_pb_model.md) to obtain the Switch\_v1.pb network model.

1. Obtain the output of the control flow operator network model.

    Switch to the directory where the **summarize\_graph** executable file is located and run the following command:

    ```bash
    ./summarize_graph --in_graph=/home/test/module/Switch_v1.pb
    ```

    If the following information is returned, the execution is successful:

    ```console
    Found 3 possible inputs: (name=x, type=float(1), shape=<unknown>) (name=y, type=float(1), shape=<unknown>) (name=z, type=float(1), shape=<unknown>)
    No variables spotted.
    Found 1 possible outputs: (name=add, op=AddV2)
    Found 0 (0) const parameters, 0 (0) variable parameters, and 0 control_edges
    Op types used: 3 Placeholder, 3 Switch, 2 AddV2, 2 Mul, 2 Square, 1 Identity, 1 Less, 1 Merge
    To use with tensorflow/tools/benchmark:benchmark_model try these arguments:
    bazel run tensorflow/tools/benchmark:benchmark_model -- --graph=/home/test/module/Switch_v1.pb --show_flops --input_layer=x,y,z --input_layer_type=float,float,float --input_layer_shape=:: --output_layer=add
    ```

2. Create the config.pbtxt output configuration file.

    Run the **vim config.pbtxt** command in any directory to create the config.pbtxt file. This example creates the file in the $HOME/module directory.

    As shown in step 1, the network model has one output. The following is a sample config.pbtxt configuration file (the following configuration file is only a sample. You need to modify the name of the output operator based on the actual situation, where fetch indicates the output):

    ```pbtxt
    fetch {
      id { node_name: "add" }
    }
    ```

    After the configuration is complete, save the file and exit.

3. Generate the configuration file of the function-type operator network model.

    Run the following command in any path to set the screen log information during the execution of the **xlacompile** command:

    ```bash
    export TF_CPP_MIN_LOG_LEVEL=0
    export TF_CPP_MIN_VLOG_LEVEL=1
    ```

    Switch to the path where the **xlacompile** executable file is located and run the following command:

    ```bash
    ./xlacompile --graph=/home/test/module/Switch_v1.pb --config=/home/test/module/config.pbtxt --output=/home/test/module/Switch_v1_v2
    ```

    If the message "Successfully convert ..." is displayed, the conversion is successful. Switch to the path specified by the **--output** parameter to view the following output files:

    ```console
    -rw-rw-r-- 1 test test 1236 Jun 20 17:13 Switch_v1_v2.pb
    -rw-rw-r-- 1 test test 4803 Jun 20 17:13 Switch_v1_v2.pbtxt
    ```

4. Generate a graph subgraph from the function-type operator network model. This file is required for subgraph generation during subsequent model conversion using the ATC tool.

    Run the following command in any path to generate a subgraph from the function-type operator network model:

    ```python
    python3.7.5 ${INSTALL_DIR}/compiler/python/func2graph/func2graph.py -m /home/test/module/Switch_v1_v2.pb
    ```

    If the following information is displayed, the generation is successful.

    ```console
    graph_def_file:  /home/test/module/graph_def_library.pbtxt
    INFO: Convert to subgraphs successfully.
    ```

5. Convert the function-type operator network model into an offline model that adapts to Ascend AI Processors.

    See [Setting Environment Variables](../overview/environment_setup.md#setting-environment-variables) to set the environment variables required for ATC tool execution, and then run the following command to convert the model:

    ```bash
    atc --model=/home/test/module/Switch_v1_v2.pb --framework=3 --output=/home/test/module/out/Switch_v1_v2_to_om --soc_version=<soc_version>
    ```

    If the following information is displayed, the model conversion is successful:

    ```console
    ATC run success, welcome to the next use.
    ```

    After the command is successfully executed, you can view the model file in the path specified by the **--output** parameter.
