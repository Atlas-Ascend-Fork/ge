# GE Overview

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-21T10:05:55.422Z pushedAt=2026-10-28T11:05:30.947Z nl=2 -->

## GE Overview

The graph engine (GE) is the control center for computational graph compilation and execution on the Ascend platform. It provides graph construction, graph compilation optimization, and graph execution control. With the capabilities of GE, algorithm models from mainstream AI frameworks such as PyTorch, TensorFlow, MindSpore, and PaddlePaddle can be uniformly converted into computational graphs (Ascend Graph) represented by Ascend IR (Ascend Intermediate Representation). Through GE's graph compilation acceleration technologies, the execution efficiency of computational graphs on Ascend hardware is significantly improved. In addition, GE provides a unified graph engine interface that supports custom graph structures, helping users quickly deploy neural network services on Ascend hardware.

**Figure 1** GE logical architecture diagram
![Illustration](../figures/ge_logical_architecture.png "GE logical architecture diagram")

## GE Graph Compilation Acceleration Technologies

Mainstream deep learning frameworks currently provide two execution modes: Eager (Eager Execution) mode and graph mode. In Eager mode, each computational operation is executed immediately after being dispatched, whereas in graph mode, all computational operations are constructed into a graph and executed in the form of a graph. Compared with dispatching computational operations one by one, graph mode provides a global view of the graph, which enables more effective simplification and optimization of computational graph operations, thereby achieving better execution performance.

As an AI compiler specifically designed for graph mode, GE effectively reduces Host-Device scheduling interactions, decreases model memory usage, improves whole-graph execution performance, and accelerates model execution efficiency on the Ascend platform through a rich set of graph compilation acceleration technologies, including computational graph optimization, multi-stream parallelism, memory reuse, and model offloading. For detailed information about GE key technologies, developers can access the following links:

- [Computational Graph Optimization](https://www.hiascend.com/en/developer/techArticles/20240621-1)
- [Multi-Stream Parallelism](https://www.hiascend.com/en/developer/techArticles/20240701-1)
- [Memory Reuse](https://www.hiascend.com/en/developer/techArticles/202407005-1)
- [Model Offloading](https://www.hiascend.com/en/developer/techArticles/20240715-1)
- [Small-Shape Operator Computation Optimization Technology](https://www.hiascend.com/en/developer/techArticles/20250911-1)

## GE Open Capabilities

- For upper-layer AI framework integration and service deployment scenarios: GE provides unified graph engine interfaces to support the integration of upper-layer graph-based open frameworks. It has adapted to mainstream AI frameworks such as PyTorch (TorchAir graph mode), TensorFlow, MindSpore, and PaddlePaddle. It also supports custom graph structures, helping users efficiently deploy neural network services on Ascend hardware.
- For whole-graph compilation and runtime optimization scenarios of AI models: GE opens up graph compilation optimization and graph execution capabilities, and supports functions such as custom graph fusion, helping users customize high-performance graph solutions.

## GE Usage Entry

GE provides different integration solutions for different user scenarios, making it easier for users to use the GE graph engine capabilities.

- **Graph mode execution of models such as ONNX/PB**

    GE provides the ATC command-line tool or the C++ Parser API, which supports converting models exported by frameworks such as PyTorch and TensorFlow (\*.onnx/\*.pb/\*.air format) into offline models adapted to AI processors (\*.om format), and executing them in graph mode. For details, see:

  - [Converting an ONNX Model Using ATC](quick_start.md#converting-an-onnx-model-using-atc).
  - [Using Parser APIs to Parse the Original Model into a Graph](../construct_graph/parse_model_to_graph_using_parser.md).

- **Using GE Graph Mode in AI Frameworks Such as PyTorch**
  - For mainstream AI frameworks such as PyTorch, TensorFlow, MindSpore, and PaddlePaddle, integration with GE is currently supported. Users only need to make a small amount of migration and adaptation to the training code to use the GE graph engine capabilities, thereby accelerating the execution of training or online inference tasks on AI processors.
    - Integrating GE into the PyTorch Framework

        PyTorch networks support integration with the GE graph engine through the TorchAir component of TorchNPU. TorchAir (Torch Ascend Intermediate Representation) is an extension library in TorchNPU that supports graph mode capabilities. It works with the [Dynamo feature](https://docs.pytorch.org/docs/main/user_guide/torch_compiler/torch.compiler_dynamo_overview.html) of PyTorch to convert PyTorch [FX](https://pytorch.org/docs/main/fx.html) (Functionalization) graphs into computational graphs expressed in Ascend IR (Intermediate Representation), and then performs operations such as computational graph compilation and optimization through GE before dispatching them to Ascend hardware for execution.

        **Figure 2** Software architecture of the Ascend platform in PyTorch graph mode
        ![Illustration](../figures/pytorch_graph_arch.png "Software architecture of the Ascend platform in PyTorch graph mode")

        The Ascend compilation backend provided by TorchAir can be passed as a parameter to the torch.compile API of PyTorch to support graph mode execution. The following is an example:

        ```C++
        # Import torch_npu first, and then import torchair.
        import torch
        import torch_npu
        import torchair

        # Define the model Model.
        class Model(torch.nn.Module):
            def __init__(self):
                super().__init__()
            def forward(self, x, y):
                return torch.add(x, y)

            # Instantiate the model.
            model = Model()

            # Obtain the default backend provided by the NPU from TorchAir.
            config = torchair.CompilerConfig()
            npu_backend = torchair.get_npu_backend(compiler_config=config)

            # Use the TorchAir backend to call the compile API to compile the model.
            opt_model = torch.compile(model, backend=npu_backend)

            # Execute the compiled model.
            x = torch.randn(2, 2)
            y = torch.randn(2, 2)
            opt_model(x, y)
        ```

        The specific usage is as follows:

        1. Migrate the original PyTorch network to the Ascend platform. For details about the migration process, see *Traditional Model Migration and Tuning Guide*.
        2. Implement graph mode inference through TorchAir. For detailed configuration, see *TorchAir User Guide*.

    - TensorFlow framework integration with GE

        TensorFlow 1.15 networks run in graph mode and support integration with the GE graph engine through the TF Adapter. The frontend TensorFlow model is converted into a computational graph expressed in Ascend IR, which is then compiled and optimized by GE and dispatched to Ascend hardware for execution. The default execution mode of TensorFlow 2.6.5 networks is Eager mode (that is, single-operator mode), in which each computational operation is executed and returned immediately after being dispatched. TensorFlow 2.6.5 provides the tf.function decorator, which wraps the TF2 operations called in a Python function into a graph for execution to gain performance benefits. Currently, for TensorFlow 2.6.5, the Ascend platform supports only accelerated execution of operators in functions decorated with tf.function on Ascend hardware, that is, only graph mode execution is supported.

        **Figure 3** Software architecture of the Ascend platform in TensorFlow graph mode
        ![Illustration](../figures/tensorflow_graph_arch.png "Software architecture of the Ascend platform in TensorFlow graph mode")

        For detailed usage of migrating an original TensorFlow 1.15 network to the Ascend platform, see *[TensorFlow 1.15 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)*.

        For detailed usage of migrating an original TensorFlow 2.6.5 network to the Ascend platform, see *[TensorFlow 2.6.5 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)*.

    - MindSpore framework integration with GE

        MindSpore networks natively support the GE graph engine. You can directly use the GE graph engine capabilities based on the MindSpore framework. For detailed usage, see [https://www.mindspore.cn/en/](https://www.mindspore.cn/en/).

    - PaddlePaddle framework integration with GE

        For detailed usage, see [PaddlePaddle Tutorial Documentation](https://www.paddlepaddle.org.cn/documentation/docs/zh/develop/hardware_support/npu/install_cn.html).

  - For other third-party frameworks, GE provides unified C++ graph engine APIs for users to complete adaptation on their own. For details, see [Using Graph Engine APIs to Construct a New Graph](../construct_graph/construct_graph_using_ge_api.md).

- **Building a Network from Scratch Using Graph Engine APIs**
  - GE provides unified C++ graph engine APIs for users to define custom graph structures and execute them on AI processors based on the graph engine. For details, see [Using Graph Engine APIs to Construct a New Graph](../construct_graph/construct_graph_using_ge_api.md).
  - GE provides a set of function-style minimalist graph construction APIs that allow you to build graphs with less code, improving ease of use while providing a certain degree of error prevention. For details, see [ES Graph Construction](../es_graph/README.md).
