# Quick Start

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-28T12:54:49.839Z pushedAt=2026-10-29T05:58:24.927Z nl=2 -->

## Development Process

![](figures/260109165520203.png)

## Environment Preparation

Supported product models: Atlas A2 series and Atlas A3 series.

- Currently, only Python 3.12 is supported. For installation instructions, see the official Python website [https://www.python.org/](https://www.python.org/).

- The Ascend-cann-toolkit development kit must be installed. For details, see *[CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum)*.

    > [!NOTE]
    > In the AI Server scenario, the installation node must be isolated using containers or virtual machines (VMs). The lifecycle of the container or VM must be consistent with that of the service process or tenant. When the lifecycle of the container or VM ends, persistent data must be cleared to avoid affecting the next service process or tenant.

- Mandatory environment variables

    After installing CANN software, when the CANN running user performs compilation and running, log in to the environment as the CANN running user and run the **source \$_\{INSTALL\_DIR\}_/set\_env.sh** command to set the environment variables. \$\{INSTALL\_DIR\} must be replaced with the storage path of the files after CANN software installation. For example, when installing as the root user, the default storage path of the installed files is /usr/local/Ascend/cann.

    ```shell
    export RESOURCE_CONFIG_PATH=numa_config.json #Used to set the storage path of the heterogeneous resource description file.
    ```

    > [!NOTE]
    >numa\_config.json For details, see [numa\_config.json Configuration](appendices.md#numa_configjson-configuration).

The preceding environment variables take effect only in the current window. To make them take effect permanently, you can write the preceding commands to the \~/.bashrc file as follows:

1. As the installation user, run **vi \~/.bashrc** in any directory, and add the preceding content to the end of the file.
2. Run the **:wq!** command to save the file and exit.
3. Run **source \~/.bashrc** to make the environment variables take effect.

## Complete Sample Reference

|Sample Name| Code Address                                                                |Scenario Description|
|--|---------------------------------------------------------------------|--|
|sample_multiple_model| [Get Sample](../../../../../../examples/dflow/python/sample_multiple_model.py) |Multiple models are concatenated and offloaded for execution. FuncProcessPoint and GraphProcessPoint are used to construct a DataFlow graph. By offloading the onnx and pb models executed by GraphProcessPoint to the device, the control-plane and data-plane interactions between the host and device are reduced, improving execution performance.|
|sample_pytorch| [Get Sample](../../../../../../examples/dflow/python/sample_pytorch.py) |sample_pytorch: DataFlow is combined with PyTorch for online model inference. The original serial execution process of preprocessing, model inference, and postprocessing is transformed into a FlowGraph that can execute asynchronously in a pipelined manner, thereby improving the overall inference throughput.|
|sample_npu_model| [Get Sample](../../../../../../examples/dflow/python/sample_npu_model.py) |An optimization of sample_pytorch that adds data offloading or model offloading for PyTorch model nodes.|
