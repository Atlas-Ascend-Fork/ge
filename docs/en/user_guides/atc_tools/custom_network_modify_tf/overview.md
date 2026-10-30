# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:19:38.560Z pushedAt=2026-10-28T08:14:10.604Z nl=2 -->

## Overview

This section describes how to use the xlacompile tool of TensorFlow to convert a network model with control flow operators (as shown in [Figure 1](#fig1)) into a network model with function-class operators (as shown in [Figure 2](#fig2)), and then use the ATC tool to convert it into an offline model adapted to the AI processor.

**Figure 1**  Network model with control flow operators<a id="fig1"></a>
![Illustration](../figures/network_model_with_control_flow_op.png "Network model with control flow operators")

**Figure 2**  Network model with function-class operators<a id="fig2"></a>
![Illustration](../figures/function_class_op_network_model.png "Network model with function-class operators")

## Prerequisites

- Ensure that the server where the ATC tool is located can connect to the network.
- Install the bazel build tool.

    Install the bazel build tool by referring to the official website at [https://docs.bazel.build/versions/master/install-ubuntu.html](https://docs.bazel.build/versions/master/install-ubuntu.html).

- Install TensorFlow and the future dependency.

    The func2graph.py script in the ATC installation path depends on TensorFlow. Run **pip3.7.5 list** to check whether TensorFlow is in the list. If it is, no installation is required. Otherwise, run the following command to install it:

    ```python
    pip3.7.5 install tensorflow==1.15 --user
    ```

    The bazel build tool depends on Python. Run **pip3.7.5 list** to check whether future and patch are installed. If they are, no installation is required. Otherwise, run the following command to install them:

    ```python
    pip3.7.5 install future --user
    pip3.7.5 install patch --user
    pip3.7.5 install numpy --user
    ```

    If the network cannot be connected when running the preceding command and the message "Could not find a version that satisfies the requirement xxx" is displayed, see [Using pip3 install Software Prompts "Could not find a version that satisfies the requirement xxx"](pip3_install_error.md) to resolve the issue.
