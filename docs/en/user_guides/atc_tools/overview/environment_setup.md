# Preparing the Environment

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-11-16T15:04:33.323Z pushedAt=2026-11-17T12:41:37.031Z nl=2 -->

## Obtaining the ATC Tool

Before model conversion, see [*CANN Software Installation*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum) to set up the environment, and ensure that the CANN Toolkit development suite package and the ops operator package are installed. (For versions 8.5.0 and later, you must install the ops operator package that matches the target AI processor; otherwise, compilation fails.) Taking the default installation path of the root user as an example, the ATC tool is installed in the "/usr/local/Ascend/cann/bin" directory.

You can deploy operator packages for multiple chips in the same development environment to implement model conversion in multi-chip scenarios. To do so, install the CANN Toolkit development suite and the operator packages of the corresponding chips (for example, chip A and chip B) in different paths, for example:

- Path 1: Install CANN Toolkit + the ops operator package of chip A
- Path 2: Install CANN Toolkit + the ops operator package of chip B

Then execute the environment variables in different paths to switch between operator packages of different chips, meeting the development and deployment requirements in multi-chip environments.

## Setting Environment Variables

> [!NOTE]
>
> - After environment variables are set using the export method, they take effect only in the current window. Users can write the preceding commands into an environment variable configuration file (such as the .bashrc file) as required.
> - During model conversion using the ATC tool, the Python libraries on which operator build depends in the "../python/site-packages" directory where the ATC tool is located are automatically written to the PYTHONPATH environment variable.
> If users introduce other Python dependencies beyond the preceding dependencies when implementing operators, add the PYTHONPATH environment variable to configure the location path of the introduced Python dependencies. Example:
> `export PYTHONPATH=_xxxx_:$PYTHONPATH`

1. **Mandatory environment variables**

    - **Setting common environment variables**

        After installing the CANN software, when the CANN running user performs compilation and running, log in to the environment as the CANN running user and execute the following environment variables:

        ```bash
        source /usr/local/Ascend/cann/set_env.sh
        #If the CANN Toolkit development suite package and the ops operator package are installed on a non-Ascend device, the following environment variables must be executed to set the path of the dynamic link library. Otherwise, execution is not required.
        export LD_LIBRARY_PATH=/usr/local/Ascend/cann/<arch>-linux/devlib:$LD_LIBRARY_PATH
        ```

        /usr/local/Ascend/ is the default installation path of the root user. Replace it with the actual path, and replace <arch\> with the specific architecture of the operating system.

    - **Setting Python-related environment variables**

        Model compilation depends on Python. Taking Python 3.7.5 as an example, run the following commands as the CANN software package running user to set the Python 3.7.5-related environment variables.

        ```bash
        #If multiple python3 versions exist in the user environment, specify python3.7.5.
        export PATH=/usr/local/python3.7.5/bin:$PATH
        #Set the path of the python3.7.5 library file.
        export LD_LIBRARY_PATH=/usr/local/python3.7.5/lib:$LD_LIBRARY_PATH
        ```

    <!-- @ref: ge/res/docs/zh/user_guides/atc_tools/overview/environment_setup_res.md#id1 -->

    The preceding environment variables take effect only in the current window. You can write the preceding commands to the \~/.bashrc file to make them take effect permanently. The procedure is as follows:

    1. As the installation user, run **vi \~/.bashrc** in any directory, and add the preceding content to the end of the file.
    2. Save the file, and run **source \~/.bashrc** to make the environment variables take effect.

2. **Optional environment variables**
    1. Log persistence, screen printing, and redirection.
        <!-- npu="950,A3,910b,910,310p,310b" id1 -->
        - **Log Persistence**:

            During the execution of the atc command, logs are persisted to the following paths by default:

            - $HOME/ascend/log/**debug**/plog/plog-_pid_\_\*.log: Debug Log.

                In the Debug Log scenario, because the default value of [--log](../CLI_options/--log.md) is null, that is, no logs are output, if the preceding path contains log information, it is log information from sources other than the atc process, such as Python-related information. To make the logs reflect atc process-related information, set [--log](../CLI_options/--log.md) to a value other than null.

            - $HOME/ascend/log/**run**/plog/plog-_pid_\_\*.log: Runtime Log.
            - $HOME/ascend/log/**security**/plog/plog-_pid_\_\*.log: Security Log.

            *pid* indicates the process ID, and "\*" indicates the timestamp when the log file was created.

        <!-- end id1 -->
        <!-- @ref: ge/res/docs/zh/user_guides/atc_tools/overview/environment_setup_res.md#id2 -->

        <!-- npu="950,A3,910b,910,310p,310b" id2 -->
        - **Log Display**:

            During the execution of the atc command, logs are not displayed on the screen by default. To display them on the screen, do the following:

            - Set the screen-printing environment variable in the current window where the atc command is executed:

                ```bash
                export ASCEND_SLOG_PRINT_TO_STDOUT=1
                ```

            - In the atc model conversion command, set the [--log](../CLI_options/--log.md) parameter (it cannot be set to null).

            For more information about logs, see *[Log Reference](https://gitcode.com/cann/runtime/blob/master/docs/en/log_ref/README.md)*.
        <!-- end id2 -->

        - **Log redirection**:

            If you do not want logs to be written to disk but redirected to a file, set the screen-printing environment variable described above before model conversion, and set the [--log](../CLI_options/--log.md) parameter in the atc command (it cannot be set to null). The following is an example. Replace xxx with the actual parameters.

            ```bash
            atc xxx --log=debug >log.txt
            ```

    2. Enable **parallel operator compilation**.

        If the network model is large, you can set the following environment variables during model conversion to enable parallel operator compilation.

        ```bash
        export TE_PARALLEL_COMPILER=xxx
        ```

        The value of the environment variable TE\_PARALLEL\_COMPILER indicates the number of operator compilation processes (configured as an integer). Replace _xx_ with the specific value. The value range is 1\~32, and the default value is 8. When the value is greater than 1, parallel operator compilation is enabled. It is recommended that the value not exceed the number of CPU cores \*80%/AI processors. The method for querying the number of AI processors is as follows:

        In the environment where AI processors are installed, run the "**npu-smi info -l**" command. The Total Count in the command output is the corresponding number.

    3. Print the **graph description information of each stage** during model conversion.

        ```bash
        export DUMP_GE_GRAPH=1
        ```

        The preceding environment variables control the amount of content in the dumped graph. The values are as follows:

        - 1: Full dump that includes edge relationships and data information.
        - 2: Basic dump that does not include data such as weights.
        - 3: Simplified dump that shows only node relationships.

        After setting the preceding environment variables, you can also set the following environment variable to control the number of dumped graphs.

        ```bash
        export DUMP_GRAPH_LEVEL=2
        ```

        This environment variable takes effect only when DUMP\_GE\_GRAPH is enabled, and its default value is 2. The following two configuration methods are supported. Both methods control the number of graphs to be persisted. You can use either one as needed. Note that the two configuration methods cannot be used together:

        - Configure a numeric value. The value options are as follows:
            - 1: Dump graphs of all stages.
            - 2: Dump graphs of the trustlist stages. For details about the trustlist graphs, see the "Trustlisted" column in [Table 1](../references/dump_graph_details.md).
            - 3: Dump the final generated graph, that is, the graph optimized and compiled by GE (Graph Engine).
            - 4: Dump the earliest generated graph, that is, the compilation entry graph delivered to the software stack after GE parses and maps operators. At this point, the graph structure has not undergone GE compilation optimization.

        - Configure a string separated by |. The configuration is as follows:

            For example, if the configuration is "aa|bb", graphs whose names contain aa and bb are dumped. aa and bb must be valid strings in the graph compilation process. Valid strings can be obtained from the full dump graphs.

        After the preceding environment variables are set, the following files are generated in the current path where the atc command is executed:

        - ge\_onnx\*.pbtxt: ONNX-based model description structure, which can be opened using visualization tools such as Netron.
        - ge\_proto\*.txt: text file stored in protobuf format. This file can be converted to a JSON file to help you locate issues. This file usually appears in pairs with ge\_onnx\*.pbtxt, but ge\_proto\*.txt contains additional string-type attribute information compared with ge\_onnx\*.pbtxt. Therefore, ge\_proto\*.txt displays more complete information. You can open either of the two files.

            Because the ge\_proto\*.txt file structure has been optimized for file size compared with ge\_onnx\*.pbtxt, setting the DUMP\_GE\_GRAPH environment variable to 2 or 3 has the same effect on the ge\_proto\*.txt file, both displaying a basic dump that does not contain data such as weights.

        - ge\_readable\*.txt: (Optional) highly readable text file similar to the Dynamo fx graph style. This type of file is generated only when the DUMP\_GRAPH\_FORMAT environment variable is set and its value includes "readable".

        Each of the preceding files corresponds to a step in the model compilation process, and each file contains all operators involved in that step. For details about dump graphs, see [dump graph details](../references/dump_graph_details.md).

    4. **For more optional environment variables, see** *[Environment Variable Reference](https://gitcode.com/cann/docs/blob/master/docs/en/env-vars/README.md)*.
