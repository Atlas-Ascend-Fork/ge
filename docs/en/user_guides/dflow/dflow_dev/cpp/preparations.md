# Preparations

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-28T13:02:27.074Z pushedAt=2026-10-29T04:45:40.243Z nl=2 -->

## Environment Preparation

Supported product models: Atlas A2 series and Atlas A3 series.

The CANN software has been installed. For details, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

> [!NOTE]
> In the AI Server scenario, the installation node should be isolated using containers or virtual machines (VMs). The lifecycle of a container or VM must be consistent with that of the business process or tenant. When the lifecycle of a container or VM ends, persistent data must be cleaned up to avoid affecting the next business process or tenant.

- Required environment variables

    After the CANN software is installed, when you use the CANN running user to compile and run, log in to the environment as the CANN running user and run the `source ${INSTALL_DIR}/set_env.sh` command to set the environment variables. $\{INSTALL\_DIR\} Replace it with the storage path of the files after the CANN software is installed. For example, if the software is installed by the root user, the default storage path of the installed files is /usr/local/Ascend/cann.

    ```shell
    export RESOURCE_CONFIG_PATH=numa_config.json #Set the storage path of the heterogeneous resource description information file.
    ```

    > [!NOTE]
    >numa\_config.json For details, see [Appendix](appendices.md).

- Optional environment variables

    ```shell
    export ASCEND_GLOBAL_LOG_LEVEL=0 #Set the log level of application logs and the log level of each module. Only debug logs are supported. 0 corresponds to the DEBUG level.
    export ASCEND_SLOG_PRINT_TO_STDOUT=1 #Whether to enable log printing. 1 indicates that log printing is enabled.
    ```

    For details, see *[Environment Variables](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md)*.

The preceding environment variables take effect only in the current window. To make them take effect permanently, you can write the preceding commands to the \~/.bashrc file as follows:

1. As the installation user, run **vi \~/.bashrc** in any directory, and add the preceding content to the end of the file.
2. Run **:wq!** to save the file and exit.
3. Run **source \~/.bashrc** to make the environment variables take effect.

## Network Analysis

To build a Graph using FlowData and FlowNode, you need to clarify the following information based on the network:

1. The number of inputs in the network, represented by FlowData.
2. The number of compute nodes in the network, represented by FlowNode.
3. The actual computation in a FlowNode is represented by ProcessPoint. There are two types of ProcessPoint: GraphPp and FunctionPp. For GraphPp, refer to *[Graph Development](../../../graph_dev/README.md)* for AscendGraph development. For FunctionPp, refer to "UDF development" in [Special Topics](special_topics.md) to customize user functions.
