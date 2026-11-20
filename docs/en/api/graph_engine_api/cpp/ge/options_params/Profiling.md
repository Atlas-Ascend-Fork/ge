# Profiling

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:57:38.539Z pushedAt=2026-10-28T07:42:19.761Z nl=2 -->

## ge.exec.profilingMode

Whether to enable the Profiling function.

- 1: Enables the Profiling function and reads the Profiling collection options from ge.exec.profilingOptions.
- 0: (Default value) Disables the Profiling function.

**Configuration example:**

```c++
{"ge.exec.profilingMode", "0"};
```

**Required/Optional**: Optional

**Effective level**: Global

## ge.exec.profilingOptions

Profiling configuration options.

**Parameter values:**

- output: Path for saving Profiling collection result files. Supports configuring an absolute path or a relative path (relative to the current path when executing the command line). The path cannot contain the following special characters: "\\n", "\\f", "\\r", "\\b", "\\t", "\\v", "\\u007F".
  - An absolute path starts with "/", for example: /home/output.
  - A relative path starts directly with a directory name, for example: output.
  - This parameter has a higher priority than ASCEND\_WORK\_PATH.
  - This path does not need to be created by the user in advance; it is automatically created during collection.

- storage\_limit: Specifies the maximum file capacity allowed in the disk directory. When the Profiling data files on the disk are about to occupy the maximum storage space set by this parameter, or the remaining total disk space is about to be fully occupied (remaining total space <= 20MB), the earliest files on the disk are deleted through aging.

    Range: \[200, 4294967295\], in MB. The unit must be specified when setting this parameter, for example, 200MB.

    If this parameter is not configured, data is not written to disk when the available disk space is less than 20 MB before collection.

- training\_trace: Switch for collecting iteration trace data, that is, software information of the training job and the AI software stack, to implement performance analysis of the training job, focusing on data related to forward and backward computation and gradient aggregation and update. When collecting forward and backward operator data, this parameter must be set to on.
- task\_trace, task\_time: Switches that control the collection of operator dispatch duration and operator execution duration. The related duration data is output to files such as task\_time, op\_summary, and op\_statistic. Values:

  - on: Enable. This is the default value and has the same effect as l1.
  - off: Disable.
  - l0: Collect operator dispatch duration and operator execution duration data. Compared with l1, because basic operator information data is not collected, the performance overhead during collection is lower, and the related duration data can be measured more accurately.
  - l1: Collect operator dispatch duration, operator execution duration data, and basic operator information data, providing more comprehensive performance analysis data.

    When training profiling mode is enabled, that is, when training Profiling data is collected, task\_trace must be set to on, and training\_trace must also be set to on.

- ge\_api: Collect duration data of dynamic shape operators in the Host scheduling phase. Values:
  - off: Disable. Defaults to off.
  - l0: Collects duration data of dynamic shape operators in the main stages of Host scheduling, enabling more accurate statistics of the related duration data.
  - l1: Collects finer-grained duration data of dynamic shape operators in the Host scheduling stage, providing more comprehensive performance analysis data.

- hccl: Controls the switch for communication data collection. The value can be on or off, and defaults to off.

    > [!NOTE]
    > This switch will be deprecated in later versions. Use the task\_time switch to control the related data collection.

- aicpu: Collects detailed information about AICPU operators, such as operator execution time and data copy time. The value can be on or off, and defaults to off.
- fp\_point: Specifies the start position of the forward operator in the training network iteration trajectory, used to record the forward computation start timestamp. The configured value is the name of the specified first forward operator. You can save the graph as a .pbtxt file through tf.io.write\_graph in the training script and obtain the name from the file to fill in. You can also leave it empty, in which case the system automatically identifies the start position of the forward operator, for example, "fp\_point":"".
- bp\_point: Specifies the end position of the backward operator in the training network iteration trajectory, recording the backward computation end timestamp. bp\_point and fp\_point can be used to calculate the forward and backward time. The configured value is the name of the specified last backward operator. You can save the graph as a .pbtxt file through tf.io.write\_graph in the training script and obtain the name from the file to fill in. You can also leave it empty, in which case the system automatically identifies the end position of the backward operator, for example, "bp\_point":"".
- aic\_metrics: AI Core performance metric collection items. The values are as follows:

  - ArithmeticUtilization: Statistics on the proportion of various computation metrics.
  - PipeUtilization: Proportion of time consumed by compute units and transfer units. This is the default value.
  - Memory: Proportion of external memory read/write instructions.
  - MemoryL0: Proportion of internal L0 memory read/write instructions.
  - MemoryUB: Proportion of internal UB memory read/write instructions.
  - ResourceConflictRatio: Proportion of pipeline queue instructions.
  - L2Cache: Number of L2 cache read/write hits and reallocations after a miss.

      <!-- npu="310p" id1 -->
      Atlas inference products do not support this parameter.
      <!-- end id1 -->

      <!-- npu="910" id2 -->
      Atlas training products do not support this parameter.
      <!-- end id2 -->

  - MemoryAccess: Bandwidth data volume of memory access by the operator on the core.

      <!-- npu="310p" id3 -->
      Atlas inference products do not support this parameter.
      <!-- end id3 -->

      <!-- npu="910" id4 -->
      Atlas training products do not support this parameter.
      <!-- end id4 -->

      <!-- npu="950" id5 -->
      Ascend 950PR&950DT products: This product does not support this parameter.
      <!-- end id5 -->

    > [!NOTE]
    >
    > You can customize the registers to be collected, for example: "aic\_metrics":"**Custom:**_0x49,0x8,0x15,0x1b,0x64,0x10_".
    > - The Custom field indicates a custom type, configured as a specific register value, with a range of \[0x1, 0x6E\].
    > - A maximum of 8 registers can be configured, separated by commas.
    > - Register values support hexadecimal or decimal format.

- l2: Controls the switch for the L2 Cache and TLB page table cache hit rate. The value can be on or off, and defaults to off.
  <!-- npu="310p" id6 -->
  - Atlas inference products: support collection of the L2 Cache hit rate.
  <!-- end id6 -->
  <!-- npu="910" id7 -->
  - Atlas training products: support collection of the L2 Cache hit rate.
  <!-- end id7 -->
  <!-- npu="910b" id8 -->
  - Atlas A2 products: support collection of the L2 Cache and TLB page table cache hit rates. To analyze the number of AI Core L2 hits, use aic-metrics=L2Cache.
  <!-- end id8 -->
  <!-- npu="A3" id9 -->
  - Atlas A3 products: support collection of the L2 Cache and TLB page table cache hit rates. To analyze the number of AI Core L2 hits, use aic-metrics=L2Cache.
  <!-- end id9 -->
  <!-- npu="950" id10 -->
  - Ascend 950PR&950DT products: support collection of the L2 Cache and TLB page table cache hit rates. To analyze the number of AI Core L2 hits, use aic-metrics=L2Cache.
  <!-- end id10 -->

- msproftx: Controls the switch for msproftx users and upper-layer framework programs to output performance data. The value can be on or off, and defaults to off.

    You need to first add the following mstx API or msproftx API to the application script. The mstx API is recommended.

- runtime\_api: Controls the collection switch for runtime API performance data. The value can be on or off, and defaults to off. It collects runtime API performance data, including the latency of synchronous and asynchronous memory copies between the Host and Device and between Devices.
- sys\_hardware\_mem\_freq: Collection switch for on-chip memory, QoS transmission bandwidth, LLC L3 cache bandwidth, accelerator bandwidth, SoC transmission bandwidth, and component memory usage. The collection content varies slightly by product. The actual results prevail. Range \[1,100\], unit Hz.

    <!-- npu="950" id11 -->
    For Ascend 950PR&950DT products, the maximum collection frequency supported by QoS and SoC is 10000, while the maximum collection frequency supported by other collection items remains 100. If the configured value exceeds the range, other collection items are collected at the maximum collection frequency of 100.
    <!-- end id11 -->

    It is known that collecting memory data in an environment with glibc earlier than 2.34 may trigger a known glibc [Bug 19329](https://sourceware.org/bugzilla/show_bug.cgi?id=19329). This issue can be resolved by upgrading the glibc version of the environment.

    <!-- npu="A3,910b,310b" id12 -->
    For the following models, after the collection task ends, you are not advised to increase the collection frequency. Otherwise, SoC transmission bandwidth data may be lost.

    <!-- npu="310b" id13 -->
    Atlas 200I/500 A2 inference products
    <!-- end id13 -->

    <!-- npu="910b" id14 -->
    Atlas A2 products
    <!-- end id14 -->

    <!-- npu="A3" id15 -->
    Atlas A3 products
    <!-- end id15 -->
    <!-- end id12 -->

- llc\_profiling: LLC Profiling collection events, which can be set to:
  - read: Read event, L3 cache read rate, defaults to read.
  - write: Write event, L3 cache write rate.

- sys\_io\_sampling\_freq: Collection frequency of NIC, ROCE, and UB bandwidth data. Range \[1,100\], in Hz.

    <!-- npu="310p" id16 -->
    Atlas inference products: do not support this parameter.
    <!-- end id16 -->

    <!-- npu="910b" id17 -->
    Atlas A2 products: support collection of NIC and ROCE
    <!-- end id17 -->

    <!-- npu="A3" id18 -->
    Atlas A3 products: support collection of NIC and ROCE
    <!-- end id18 -->

    <!-- npu="950" id19 -->
    Ascend 950PR&950DT products: support collection of UB bandwidth data
    <!-- end id19 -->

- sys\_interconnection\_freq: Collection frequency of collective communication bandwidth data (HCCS), collective communication hardware acceleration unit (CCU) bandwidth data, SIO data, PCIe data, UB bandwidth data, and inter-chip transmission bandwidth information. Range \[1,50\], in Hz.
  <!-- npu="910" id20 -->
  - Atlas training products: support collection of HCCS and PCIe data.
  <!-- end id20 -->
  <!-- npu="910b" id21 -->
  - Atlas A2 products: support collection of HCCS, PCIe data, and inter-chip transmission bandwidth information.
  <!-- end id21 -->
  <!-- npu="A3" id22 -->
  - Atlas A3 products: support collection of HCCS, PCIe data, inter-chip transmission bandwidth information, and SIO data.
  <!-- end id22 -->
  <!-- npu="950" id23 -->
  - Ascend 950PR&950DT products: support collection of PCIe data, inter-chip transmission bandwidth information, CCU bandwidth data, SIO data, and UB bandwidth data.
  <!-- end id23 -->

- dvpp\_freq: DVPP collection frequency. Range \[1,100\], unit Hz.
- instr\_profiling: Bandwidth and latency collection switch for AI Core and AI Vector. Values: on/off. Default: off.
  <!-- npu="910" id24 -->
  - Atlas training products: This function is not supported.
  <!-- end id24 -->
  <!-- npu="910b" id25 -->
  - Atlas A2 products: This switch is not supported. Use instr\_profiling\_freq to control this function.
  <!-- end id25 -->
  <!-- npu="A3" id26 -->
  - Atlas A3 products: This switch is not supported. Use instr\_profiling\_freq to control this function.
  <!-- end id26 -->
  <!-- npu="950" id27 -->
  - Ascend 950PR&950DT products: Supported, but the statistics may be inaccurate because the statistics time of the last instruction segment is excessively long. It is recommended to use the msprof op method for collection.
  <!-- end id27 -->

- instr\_profiling\_freq: Bandwidth and latency collection switch for AI Core and AI Vector. Configuring the collection frequency enables the related collection capability. Range \[300,30000\], unit Hz.
  <!-- npu="910" id28 -->
  - Atlas training products: This function is not supported.
  <!-- end id28 -->
  <!-- npu="910b" id29 -->
  - Atlas A2 products: Supported, but instr\_profiling\_freq is mutually exclusive with training\_trace, task\_trace, hccl, aicpu, fp\_point, bp\_point, aic\_metrics, l2, task\_time, and runtime\_api, and they cannot be executed simultaneously.
  <!-- end id29 -->
  <!-- npu="A3" id30 -->
  - Atlas A3 products: Supported, but instr\_profiling\_freq is mutually exclusive with training\_trace, task\_trace, hccl, aicpu, fp\_point, bp\_point, aic\_metrics, l2, task\_time, and runtime\_api, and cannot be executed simultaneously.
  <!-- end id30 -->
  <!-- npu="950" id31 -->
  - Ascend 950PR&950DT products: This switch is not supported. Control this function through instr\_profiling.
  <!-- end id31 -->

- host\_sys: Switch for Host-side performance data collection. The value can be one or more of the following, separated by commas when multiple are selected, for example "host\_sys": "cpu,mem".
  - cpu: Process-level CPU utilization.
  - mem: Process-level memory utilization.
  - disk: Process-level disk I/O utilization. Collecting Host-side disk performance data requires installing the third-party open-source tool iotop, and collecting osrt performance data requires installing the third-party open-source tools perf and ltrace.
  - network: System-level network I/O utilization.
  - osrt: Process-level syscall and pthreadcall.

- host\_sys\_usage: Collect CPU and memory data of the Host-side system and all processes. The value includes cpu and mem, and can be one or more of them, separated by commas when multiple are selected.
- host\_sys\_usage\_freq: Configure the collection frequency of CPU and memory data of the Host-side system and all processes. Range \[1,50\], default value 50, unit Hz.

**Configuration example:**

```c++
std::map<ge::AscendString, ge::AscendString> ge_options = {{"ge.exec.deviceId", "0"},
                                  {"ge.graphRunMode", "1"},
                                  {"ge.exec.profilingMode", "1"},
                                  {"ge.exec.profilingOptions", R"({"output":"/tmp/profiling","training_trace":"on","fp_point":"resnet_model/conv2d/Conv2Dresnet_model/batch_normalization/FusedBatchNormV3_Reduce","bp_point":"gradients/AddN_70"})"}};
```

**Required/Optional**: Optional

**Effective level**: Global
