# Compiling Executable Files

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:16:21.788Z pushedAt=2026-10-28T08:12:17.261Z nl=2 -->

Perform the following operations as the ATC software package installation user:

1. Download the TensorFlow source code from [https://github.com/tensorflow/tensorflow/archive/v1.15.0.tar.gz](https://github.com/tensorflow/tensorflow/archive/v1.15.0.tar.gz), and then upload the downloaded source code to any directory on the Linux server where the ATC tool is located.
2. Log in to the Linux server, switch to the directory where the TensorFlow source code is located, and run the following command to decompress the source code package:

    ```bash
    tar -zxvf tensorflow-1.15.0.tar.gz
    ```

3. Go to the decompressed tensorflow-1.15.0 directory and install the patch.

    See [Obtaining the xlacompile.patch Patch File](get_xlacompile_patch.md) to obtain the **xlacompile.patch** patch, upload it to the tensorflow-1.15.0 directory on the Linux server, and run the following command to install the patch:

    ```bash
    patch -p1 < xlacompile.patch
    ```

4. Switch to the tensorflow-1.15.0 directory and run the following command to compile the **xlacompile** tool:

    ```bash
    bazel build --config=monolithic //tensorflow/compiler/aot:xlacompile
    ```

    If information similar to the following is displayed, the compilation is successful. The compilation takes about 10 minutes. If the compilation fails, see [Resolving the "An error occurred during the fetch of repository 'io\_bazel\_rules\_docker'", compilation failed Error When Compiling with the Bazel Tool](./bazel_compile_error.md).

    ```console
    Target //tensorflow/compiler/aot:xlacompile up-to-date:
      bazel-bin/tensorflow/compiler/aot/xlacompile
    INFO: Elapsed time: 214.550s, Critical Path: 73.38s
    INFO: 1511 processes: 1511 local.
    INFO: Build completed successfully, 1513 total actions
    ```

    After the compilation is successful, the **xlacompile** executable file is generated in the $HOME/.cache/bazel/\_bazel\__test_/_abd37aaac8a380ca5a3f13938322fcb2_/external/org\_tensorflow/bazel-out/k8-opt/bin/tensorflow/compiler/aot path (this path is only an example; use the path generated in your actual compilation).

    The **xlacompile** executable file is used to convert a network model with control flow operators into a network model with function-type operators.

5. Switch to the tensorflow-1.15.0 directory and run the following command to compile the **summarize\_graph** tool:

    ```bash
    bazel build --config=monolithic -c opt //tensorflow/tools/graph_transforms:summarize_graph
    ```

    If information similar to the following is displayed, the compilation is successful:

    ```console
    Target //tensorflow/tools/graph_transforms:summarize_graph up-to-date:
      bazel-bin/tensorflow/tools/graph_transforms/summarize_graph
    INFO: Elapsed time: 70.474s, Critical Path: 53.16s
    INFO: 1028 processes: 1028 local.
    INFO: Build completed successfully, 1053 total actions
    ```

    After successful compilation, the **summarize\_graph** executable file is generated in the $HOME/.cache/bazel/\_bazel\__test_/_abd37aaac8a380ca5a3f13938322fcb2_/execroot/org\_tensorflow/bazel-out/k8-opt/bin/tensorflow/tools/graph\_transforms path (this path is only an example; use the path generated in your actual compilation).

    The **summarize\_graph** executable file is used to view the input and output nodes of a network model with control flow operators, making it easier for you to construct the config.pbtxt input and output configuration file.
