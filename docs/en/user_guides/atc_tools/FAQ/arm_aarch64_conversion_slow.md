# Model Conversion Takes a Long Time When the Development Environment Architecture Is Arm (aarch64)

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:21:09.741Z pushedAt=2026-10-28T08:41:37.709Z nl=2 -->

## Symptoms

When the development environment OS and architecture are Arm (aarch64), model conversion takes a long time.

## Cause Analysis

The architecture kernel does not specify the CPU core with the optimal number.

## Solution

You can use the numactl tool to specify CPU cores before performing model conversion. The procedure is as follows:

1. Log in to the development environment as the ATC installation user and run the **su root** command to switch to the root user.
2. After ensuring that the development environment is connected to the network, run the following command to install the numactl tool.

    ```bash
    yum -y install numactl
    ```

3. Switch to the ATC installation user and run the following command to specify CPU cores 16 to 31 through **numactl -C** for model conversion.

    It is recommended to specify CPU cores 16 to 31 for better processing performance. You can also modify them based on the actual situation.

    ```bash
    numactl -C 16-31 --localalloc [args]
    ```

    \[args\] Replace this with the specific ATC model conversion command.
