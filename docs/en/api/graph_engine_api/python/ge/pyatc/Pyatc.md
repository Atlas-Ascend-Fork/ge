# Pyatc APIs

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-10-15T04:09:33.017Z pushedAt=2026-10-16T10:53:07.691Z -->

## Applicable Products

Supported on all chips.

## Description

**pyatc** is a Python wrapper interface for the ATC (Ascend Tensor Compiler) offline model compilation tool. It is fully equivalent to the ATC command-line tool in functionality, aiming to provide Python users with a more flexible integration experience.

The differences between **pyatc** and the ATC command-line tool are as follows:

- ATC command-line tool: It is started as an independent subprocess. The Python interpreter used during compilation is resolved by the atc process itself, which may be inconsistent with the user's current terminal environment (such as python3), easily causing dependency conflicts or path issues.
- **pyatc**: It executes the compilation logic directly in the current main process and reuses the user's current Python interpreter and environment variables, ensuring that the compilation environment is fully consistent with the runtime environment and avoiding environment isolation issues between processes.

## Invocation

First, see [Environment Preparation](../../../../../user_guides/graph_dev/overview/environment_setup.md) to complete the configuration of the relevant environment variables, and then select one of the following entries based on the actual usage scenario:

- Use the command-line entry:

    ```bash
    pyatc [parameters]
    ```

- Use the Python module entry:

    ```bash
    python3 -m ge.pyatc [parameters]
    ```

## Parameters

The parameters of pyatc are exactly the same as those of the ATC command-line tool. For detailed function descriptions, see [ATC Offline Model Compilation Tool](../../../../../user_guides/atc_tools/README.md).

## Return Value

After the call completes, the process exit code indicates the execution result:

- **0**: Execution succeeded.
- Non-zero: Execution failed, and the error information is output to the standard output.

## Examples

```bash
pyatc --model=resnet50.onnx --framework=5 --soc_version=<soc_version> --output=resnet50
```

<soc\_version\> For the query method, see [--soc_version](../../../../../user_guides/atc_tools/CLI_options/--soc_version.md).
