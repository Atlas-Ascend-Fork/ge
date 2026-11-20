# Quick Start

<!-- md-trans-meta sourceCommit=f2e33566b0420150e0470a13f9852d17f1cfd42b translatedAt=2026-11-16T09:00:09.740Z pushedAt=2026-11-17T03:39:11.996Z nl=2 -->

## Development Process

![](figures/260109165520203.png)

## Environment Preparation

Supported product models: Atlas A2 series and Atlas A3 series.

- Currently, only Python 3.12 is supported. For installation methods, see the official Python website [https://www.python.org/](https://www.python.org/).

- The development kit package Ascend-cann-toolkit has been installed. For details, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

    > [!NOTE]Note
    > In the AI Server scenario, the installation node should be isolated using containers or virtual machines (VMs). The lifecycle of a container or VM must be consistent with that of the business process or tenant. When the lifecycle of a container or VM ends, persistent data must be cleaned up to avoid affecting the next business process or tenant.

- Required environment variables

    After the CANN software is installed, when the CANN running user compiles and runs, log in to the environment as the CANN running user and run the **source \$_\{INSTALL\_DIR\}_/set\_env.sh** command to set the environment variables. \$\{INSTALL\_DIR\} Replace it with the storage path of the files after the CANN software is installed. For example, if the software is installed by the root user, the default storage path of the installed files is /usr/local/Ascend/cann.

    ```shell
    export RESOURCE_CONFIG_PATH=numa_config.json # Set the storage path of the heterogeneous resource description file.
    ```

    > [!NOTE]Note
    >numa\_config.json For details, see [numa\_config.json Configuration](appendices.md#numa_configjson-configuration).

The above environment variables take effect only in the current window. To make them take effect permanently, add the above commands to the \~/.bashrc file as follows:

1. As the installation user, run **vi \~/.bashrc** in any directory, and add the above content to the end of the file.
2. Run **:wq!** to save the file and exit.
3. Run **source \~/.bashrc** to make the environment variables take effect.

## Complete Example Reference

| Sample Name | Code Address | Scenario Description |
|--|---------------------------------------------------------------------|--|
|sample_multiple_model| [Get Sample](../../../../../../examples/dflow/python/sample_multiple_model.py) |Multiple models are concatenated and offloaded for execution. FuncProcessPoint and GraphProcessPoint are used to construct a DataFlow graph. By offloading the onnx and pb models executed by GraphProcessPoint to the device, the control-plane and data-plane interactions between the host and the device are reduced, improving execution performance.|
|sample_pytorch| [Get Sample](../../../../../../examples/dflow/python/sample_pytorch.py) |sample_pytorch: DataFlow is combined with PyTorch for online model inference. The original serial execution process of preprocessing, model inference, and postprocessing is transformed into a FlowGraph that can execute asynchronously in a pipelined manner, thereby improving the overall inference throughput.|
|sample_npu_model| [Get Sample](../../../../../../examples/dflow/python/sample_npu_model.py) |An optimization of sample_pytorch that adds data offloading or model offloading for PyTorch model nodes.|
