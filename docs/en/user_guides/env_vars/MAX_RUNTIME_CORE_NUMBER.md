# MAX\_RUNTIME\_CORE\_NUMBER

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-20T12:52:36.681Z pushedAt=2026-10-28T11:36:44.446Z nl=2 -->

## Description

In training and online inference scenarios, for networks executed in dynamic shape graph mode, you can set this environment variable to enable multithread task scheduling of the graph executor (host side).

This environment variable must be set to an integer. When the value is greater than or equal to 2, multithreading is enabled for host-side graph execution scheduling tasks, and the number of threads is the same as the value of this environment variable.

If you use this environment variable, it is recommended that you set it to 3 to achieve better performance.

## Configuration Example

```bash
export MAX_RUNTIME_CORE_NUMBER=3
```

## Constraints

1. This environment variable is used only in graph mode scenarios.
2. When this environment variable is configured, the number of CPU cores available for execution on the host must be greater than or equal to 2.
3. During graph execution, bind the host process to the specified CPU cores before the first iteration to achieve better performance.

    The following is an example of core binding in PyTorch graph mode. Assume that the host has 192 CPU cores and 8 processes, and each process is bound to 24 CPU cores. The code snippet is as follows:

    ```bash
    import os
    import psutil
    import torch
    # Bind cores
    cpu_ids_array = [range(i*24, (i+1)*24) for i in range(0,8)]
    rank_os_par_array = [f"kernel_bond_rank{i}" for i in range(0,8)]
    rank_no = int(torch.distributed.get_rank())
    kernel_bond_os_par = int(os.getenv(rank_os_par_array[rank_no], "0"))
    if kernel_bond_os_par == 0:
       logging.info(f"rank{rank_no} is about to bound cpu kernels")
       pid = os.getpid()
       process = psutil.Process(pid)
       process.cpu_affinity(cpu_ids_array[rank_no])
       os.environ[rank_os_par_array[rank_no]] = str(kernel_bond_os_par + 1)
    ```

## Applicable Products

<!-- npu="950" id1 -->
Ascend 950PR&950DT products: Supported
<!-- end id1 -->

<!-- npu="A3" id2 -->
Atlas A3 products: Supported
<!-- end id2 -->

<!-- npu="910b" id3 -->
Atlas A2 products: Supported
<!-- end id3 -->

<!-- npu="310b" id6 -->
Atlas 200I/500 A2 inference products: Not supported
<!-- end id6 -->

<!-- npu="310p" id4 -->
Atlas inference products: Supported
<!-- end id4 -->

<!-- npu="910" id5 -->
Atlas training products: Supported
<!-- end id5 -->
