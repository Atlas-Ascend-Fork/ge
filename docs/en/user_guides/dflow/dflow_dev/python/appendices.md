# Appendix

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-28T12:48:00.841Z pushedAt=2026-10-29T05:42:59.661Z nl=2 -->

## DataFlow Framework Introduction

**DataFlow Framework Structure**

**Figure 1**  DataFlow framework structure
![](figures/dataflow_framework_architecture.png "DataFlow Framework Structure")

The DataFlow structure can be divided into three layers, which are described as follows.

- DataFlow API: responsible for heterogeneous computing flow expression. App developers use the DataFlow API to express the data flow of their business and generate a computation graph based on Graph IR. For detailed information about data flow expression, see "Data Flow Expression".
- Heterogeneous Parallel Compiler: responsible for heterogeneous computation graph compilation. It performs graph partitioning, graph optimization, and graph deployment based on the composition relationships and connection topology of heterogeneous computing units, and generates the execution Bin for each heterogeneous computing unit.
- Heterogeneous Runtime: schedules the execution Bin to the corresponding heterogeneous execution unit for execution based on the compilation output of the heterogeneous computation graph. For detailed information about the Heterogeneous Runtime, see "Heterogeneous Runtime".

**Data Flow Expression**

The rules and constraints for data flow graph expression are as follows.

- A FlowGraph is a DAG (Directed Acyclic Graph). The data flow is directed and cyclic expression is not allowed.
- FlowGraph without input or output is not supported.
- Data transferred between nodes may be shared by multiple nodes and must remain immutable. Therefore, computing nodes are not allowed to modify input data, and copying is required when necessary.
- Data flow supports one-to-many, which means copying the same data and distributing it to multiple Nodes. However, multiple outputs of the same node are sent to the same input.
      ![](figures/260109170920368.png)

- It supports expressing calls to GraphProcessPoint for execution, implementing control flow expressions such as loop iteration. The branch control logic is as follows.
      ![](figures/250530151714622.png)

**Heterogeneous Runtime**

Heterogeneous Runtime provides different executors for CPU and NPU heterogeneous nodes, including the following.

- CPU graph executor: deployed on CPU nodes to execute computation graphs.
- NPU graph executor: deployed on NPU to execute computation graphs.
- UDF executor: a custom Func execution engine that can be flexibly deployed on a CPU node or use the AICPU on the NPU.

Each executor is deployed as an independent process. Data is exchanged between executor processes in the following ways: shared memory and shared queues are used within the same node, and RDMA/UB high-performance communication is used across nodes.

The production, consumption, one-to-many distribution, and cross-node data communication of data flows between executors are uniformly proxied and forwarded by FlowGW (data flow gateway).

![](figures/260109171126678.png)

## Mbuf Multiple Memory Pools

**Feature Overview**

Mbuf is a type of shared memory that consists of two parts: MbufHead and MbufData, as shown in Table 1.

**Table 1**  Mbuf introduction

|Data Type|Description|
|--|--|
|MbufHead|The data header, which contains the following types of data: basic data content, such as DataType, DataSize, and Shape; key information involved in data transfer, such as framework-related information including retCode and routeLabel; and other user-configurable fields, such as starttime, endtime, and userdata.|
|MbufData|A segment of memory on Mbuf that stores the actual valid data content.|

In the memory allocation scenario of DataFlow, Mbuf is the unified memory management mechanism of the driver. All FlowMsgs are generated and used in the Mbuf pool. Each FlowMsg allocates a segment of Mbuf memory:

```sh
sizeMbuf  =
       sizeof(MbufData) // Actual memory size contained in FlowMsg
     + sizeof(MbufHead) // Size of MbufHead, which is actually 1024B
```

> [!NOTE]
>
>FlowMsg is the carrier for information transfer between nodes on the FlowGraph, and is used to process the input and output operations of FlowFunc. FlowMsg uses the shared memory Mbuf to store information.

**Background**

In scenarios where a user has a large number of UDF processes and each UDF process may allocate a large amount of small memory, it is recommended to use Mbuf multiple memory pools. An example is provided as follows.

Assume that the total size of the Mbuf memory pool is 10 GB. When Mbuf multiple memory pools are not used, contiguous large memory blocks cannot be allocated due to memory fragmentation. The detailed information is as follows.

![](figures/250311111739664.png)

1. At time1, multiple large memory blocks (3 GB) and multiple small memory blocks (2 MB) are allocated.
2. At time2, the large memory is no longer used and is released to the memory pool, becoming idle. At this point, from the overall status of the memory pool, the overall memory usage is: 3 GB (idle) + 2 MB (allocated) + 3 GB (idle) + 2 MB (allocated) + 3 GB (idle) + 2 MB (allocated), and the available memory is 9 GB+.
3. At time3, although the available memory is 9 GB+, the allocation of 5 GB of contiguous memory fails due to the presence of memory fragmentation (2 MB memory blocks).

**Example**

For the preceding scenario, an additional memory pool configuration is required to handle the memory fragmentation issue. The main method is to configure Mbuf as a configurable multiple memory pool system, placing small memory into a dedicated small memory pool.

The following figure shows an example of Mbuf multiple memory pools.

![](figures/250311111739664-0.png)

1. At time1, multiple large memory blocks (3 GB) and multiple small memory blocks (2 MB) are allocated.
2. At time2, the large memory is no longer used and is released back to the memory pool, becoming idle. At this point, from the overall status of the memory pool, the overall memory usage is: 3 GB (idle) + 3 GB (idle) + 3 GB (idle) + 2 MB (allocated) + 2 MB (allocated). The available memory in memory pool 1 is 9 GB+, and the available memory in memory pool 2 is 1 GB+.

3. At time3, the available contiguous memory in memory pool 1 is 9 GB+, so allocating 5 GB of memory succeeds.

This solution is implemented by configuring "ge.flowGraphMemMaxSize". For a configuration example, see Example 2 in Table 2.

**Table 2** Configuration examples

|Example Name|Example Description|Example Code|
|--|--|--|
|Example 1|Allocate one memory pool with a size of 10 GB, with no limit on the allocated memory size.|{"ge.flowGraphMemMaxSize", "10737418240"}|
|Example 2|Allocate two memory pools.<br>- Memory pool 1 has a size of 10 GB, with no limit on the allocated memory size.<br>- Memory pool 2 has a size of 2 GB, with a maximum allocatable size of 2 MB.<br>When the allocated memory is <= 2 MB, memory is preferentially allocated from memory pool 2. When the allocated memory is > 2 MB, memory can only be allocated from memory pool 1.|{"ge.flowGraphMemMaxSize", "10737418240,2147483648:2097152"}|
|Example 3|Allocate three memory pools.<br>- Memory pool 1 has a size of 10 GB, with no limit on the allocated memory size.<br>- Memory pool 2 has a size of 2 GB, with a maximum allocatable size of 20 MB.<br>- Memory pool 3 has a size of 1 GB, with a maximum allocatable size of 2 MB.<br>When allocating memory, the corresponding memory pool is preferentially matched based on the allocated memory size. For example, when the allocated memory is <= 2 MB, memory is preferentially allocated from memory pool 3. When 2 MB < allocated memory <= 20 MB, memory is preferentially allocated from memory pool 2. When the allocated memory is > 20 MB, memory can only be allocated from memory pool 3.|{"ge.flowGraphMemMaxSize", "10737418240,2147483648:20971520,1073741824:2097152"}|

> [!NOTE] Description
><br>The specific meaning of ge.flowGraphMemMaxSize is as follows.
><br>In the FlowGraph scenario, this parameter specifies the memory configuration available to the network, which can be specified based on the network size. Unit: Byte. Multiple memory pool configurations are supported, with a maximum of 128 memory pools. The configuration format is "memory pool 1 size:allocation limit,memory pool 2 size:allocation limit,...,memory pool n size:allocation limit". If not set, a single memory pool with a size of 10 GB and no limit on the allocated memory size is used by default.
><br>The value range of the memory pool size is \[1024, 274877906944\], in bytes. The total size of all memory pools cannot exceed 256 GB, and is subject to hardware limitations in actual use.
><br>The value range of the allocation limit is 0 or \[1024,2147483648\], in bytes. It is an optional configuration and defaults to 0, which means no limit.

## numa\_config.json Configuration

numa\_config.json The meaning of all fields is as shown in Table 1.

**Table 1** Field Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|**Cluster**|-|-|-|
|cluster_nodes|Array of cluster_node. For details about cluster_node, see Table 2.|Yes|Description of cluster resource information.|
|nodes_topology|nodes_topology. For details about nodes_topology, see Table 5.|No|Cross-node communication topology, for example, the parameter-plane communication topology between two AISERVERs.|
|**node_def: common attributes of nodes of the same type in a cluster**|-|-|-|
|node_type|String|Yes|Node type. Example: ATLAS900 for the Atlas A2 products.|
|resource_type|String|Yes|Architecture and CPU resource type on which UDF can be deployed. The value is X86 or Aarch.|
|support_links|String|Yes|Intra-node communication mode, for example, [HCCS,PCIE,ROCE].|
|item_type|String|Yes|Accelerator card type within a node. Example: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, and Ascend*xxx*B4 for the Atlas A2 products; Ascend910_*xx* for the Atlas A3 products.|
|inter_item_memory_access_mode|String|Yes|Address access mode between different devices in a server. The value is NUMA or UMA.|
|h2d_bw|String|No|Bandwidth between HOST and DEVICE, for example, PCIE:100Gb.|
|item_topology|item_topology. For details about item_topology, see Table 6.|No|Interconnection information between different accelerator cards within a node. In the Atlas A2 products cloud scenario, accelerator cards are allocated in units of 1/2/4/8 cards. Therefore, if there is only one card, multi-card topology is not involved.|
|**item_def: common attributes of accelerator cards of the same type in a node**|-|-|-|
|item_type|String|Yes|Accelerator card type within a node. Example: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, and Ascend*xxx*B4 for the Atlas A2 products.|
|memory|String|Yes|Total memory of the entire chip. In the Atlas A2 products scenario, configure as [DDR:64GB]. In the Atlas A3 products scenario, configure as [DDR:31GB].|
|aic_type|String|Yes|Type and number of compute cores of the accelerator card, for example, [DAVINCI_V100:32].|
|resource_type|String|Yes|Ascend resource type on which UDF can be deployed. Configure as Ascend.|

**Table 2** cluster\_node Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|node_id|Integer|Yes|Node number in the cluster. Generally, 0 is used as the primary node.|
|node_type|String|Yes|Node type. Example: ATLAS900 for the Atlas A2 products.|
|ipaddr|String|Yes|IP address for control-plane communication of the node, for example, the HOST IP of a training server.|
|port|Integer|Yes|Port for control-plane communication of the node.|
|data_panel|NetworkInfo|No|Data-plane communication information.|
|memory|Integer|No|Available memory on the application HOST.|
|is_local|BOOL|No|Whether this node in this file is the local machine when multiple nodes form a cluster.|
|auth_lib_path|String|No|Library file that DataFlow needs to dlopen. The auth_lib_path field is written by CaaS on the cloud assistance side. The primary and secondary scheduling processes are responsible for reading and loading the .so file and calling the related APIs.|
|deploy_res_path|String|No|Path for storing model files. Default path for the root user: /root/runtime/deploy_res. Default path for non-root users: ${home}/runtime/deploy_res.|
|item_list|Array of item_info. For details about item_info, see Table 4.|Yes|Accelerator cards that execute this JOB, orchestrated by cloud resource management.|

**Table 3** NetworkInfo Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|avail_ports|Array of String|No|Range of data-plane communication port numbers available to this application instance on each node. The services carried are as follows. FlowGW asynchronous point-to-point communication: HOST FlowGW and Device FlowGW communicate over PCIE and do not occupy HOST OS port numbers. FlowGW communication between devices requires a unique port number to be allocated for communication. Collective communication: send/recv point-to-point graph entry and ranktbl collective communication. FlowGW communication between multiple devices supports the RDMA protocol. The default port number range allocated between multiple devices is 16666 to 32767, which can be customized by users.|

**Table 4** Item\_info Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Logical ID of the accelerator card within a node.|
|device_id|Integer|Yes|Physical ID of the accelerator card within a node.|
|ipaddr|String|No|IP address or network segment information used for communication.|

**Table 5**  nodes\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|type|String|Yes|The communication library determines the communication algorithm to use based on the topology type, such as star.|
|protocol|String|No|Communication protocol type between nodes. The default value is RDMA. This field does not need to be configured for a single server. Example: RDMA:200Gb.|
|topos|Array of plane. For details about plane, see Table 8.|Yes|Interconnection plane information of accelerator cards between nodes.|

**Table 6**  item\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|links_mode|String|Yes|Communication mode and bandwidth between different accelerator cards within a node, such as HCCS:128Gb.|
|links|Array of item_pair. For details about item_pair, see Table 7.|Yes|Interconnection topology between different accelerator cards within a node, such as [0, 1].|

**Table 7**  item\_pair Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Accelerator card ID.|
|pair_item_id|Integer|Yes|ID of the paired accelerator card.|

**Table 8**  plane description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|plane_id|Integer|Yes|Index of the communication plane between servers (provided by the cloud).|
|devices|Array of int list|Yes|List of accelerator cards that belong to this plane.|

<!-- npu="910b" id9 -->

**numa\_config.json Example for the Atlas A2 Products**

The numa\_config.json example is as follows. For the meaning of all fields, see Table 1.

```python
{
  "cluster":[
    {
      "cluster_nodes": [
        {
          "node_id":0,
          "node_type": "ATLAS900",
          "ipaddr": "xx.xx.xx.xx",
          "port":2509,
          "memory":25165824,
          "is_local":true,
          "auth_lib_path": "/opt/huawei/config/ascend_config/dataflow/lib/libcaas_dataflow_auth.so",
          "deploy_res_path": "/home/",
          "item_list" : [
           {
             "item_id":0,
             "device_id":0,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":1,
             "device_id":1,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":2,
             "device_id":2,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":3,
             "device_id":3,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":4,
             "device_id":4,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":5,
             "device_id":5,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":6,
             "device_id":6,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":7,
             "device_id":7,
             "ipaddr":"xx.xx.xx.xx"
           }
          ]
        }
      ],
      "nodes_topology":
      {
        "protocol": "RDMA:200Gb",
        "type" : "star",
        "topos" : [
          {
            "plane_id" : 0,
            "devices" : [0, 1, 2, 3, 4, 5, 6, 7]
          },
        ]
      }
    }
  ],
  "node_def": [
    {
      "node_type": "ATLAS900" ,
      "resource_type": "Aarch",
      "support_links": "[HCCS,PCIE,ROCE]" ,
      "item_type": "AscendxxxB1",
      "inter_item_memory_access_mode": "[NUMA]",
      "h2d_bw": "PCIE:100Gb",
      "item_topology": [
        {
          "links_mode": "HCCS",
          "links": [ [0,1],[0,2],[0,3],[0,4],[0,5],[0,6],[0,7],
                   [1,2],[1,3],[1,4],[1,5],[1,6],[1,7],
                   [2,3],[2,4],[2,5],[2,6],[2,7],
                   [3,4],[3,5],[3,6],[3,7],
                   [4,5],[4,6],[4,7],
                   [5,6],[5,7],
                   [6,7]]
        }
      ]
    }
  ],
  "item_def": [
    {
      "item_type": "AscendxxxB1",
      "resource_type": "Ascend",
      "memory": "[DDR:64GB]",
      "aic_type": "[DAVINCI_V100:32]"
    }
  ]
}
```

<!-- end id9 -->

<!-- npu="A3" id10 -->

**numa\_config.json Example for the Atlas A3 Products**

The numa\_config.json example is as follows. The meaning of all fields is shown in Table 1.

```python
{
  "cluster":[
    {
      "cluster_nodes": [
        {
          "node_id":0,
          "node_type": "ATLAS800",
          "ipaddr": "xx.xx.xx.xx",
          "port":2509,
          "memory":25165824,
          "is_local":true,
          "auth_lib_path": "/opt/huawei/config/ascend_config/dataflow/lib/libcaas_dataflow_auth.so",
          "deploy_res_path": "/home/",
          "item_list" : [
           {
             "item_id":0,
             "device_id":0,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":1,
             "device_id":1,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":2,
             "device_id":2,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":3,
             "device_id":3,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":4,
             "device_id":4,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":5,
             "device_id":5,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":6,
             "device_id":6,
             "ipaddr":"xx.xx.xx.xx"
           },
           {
             "item_id":7,
             "device_id":7,
             "ipaddr":"xx.xx.xx.xx"
           }
          ]
        }
      ],
      "nodes_topology":
      {
        "protocol": "RDMA:200Gb",
        "type" : "star",
        "topos" : [
          {
            "plane_id" : 0,
            "devices" : [0, 1, 2, 3, 4, 5, 6, 7]
          },
        ]
      }
    }
  ],
  "node_def": [
    {
      "node_type": "ATLAS900" ,
      "resource_type": "Aarch",
      "support_links": "[HCCS,PCIE,ROCE]" ,
      "item_type": "AscendxxxB1",
      "inter_item_memory_access_mode": "[NUMA]",
      "h2d_bw": "PCIE:100Gb",
      "item_topology": [
        {
          "links_mode": "HCCS",
          "links": [ [0,1],[0,2],[0,3],[0,4],[0,5],[0,6],[0,7],
                   [1,2],[1,3],[1,4],[1,5],[1,6],[1,7],
                   [2,3],[2,4],[2,5],[2,6],[2,7],
                   [3,4],[3,5],[3,6],[3,7],
                   [4,5],[4,6],[4,7],
                   [5,6],[5,7],
                   [6,7]]
        }
      ]
    }
  ],
  "item_def": [
    {
      "item_type": "Ascend910_xx",
      "resource_type": "Ascend",
      "memory": "[DDR:31GB]",
      "aic_type": "[DAVINCI_V100:32]"
    }
  ]
}
```

<!-- end id10 -->

## UDF Python Project Creation Tool

This tool creates a UDF Python project template, including the corresponding folders and files. You can develop functions based on this template, which reduces the manual creation workload.

The tool is stored in the following path: \$\{install\_path\}/cann/python/site-packages/dataflow/tools/create\_func\_ws.py, where $\{install\_path\} is the installation directory of the CANN software, which is "/usr/local/Ascend" by default.

The usage is as follows.

1. Setting environment variables.

    After installing the CANN software, when the CANN running user performs compilation and running, log in to the environment as the CANN running user and run the **source \$*\{INSTALL\_DIR\}*/set\_env.sh** command to set the environment variables. Replace $\{INSTALL\_DIR\} with the storage path of the files after the CANN software is installed. For example, if the software is installed by the root user, the default storage path of the files after installation is /usr/local/Ascend/cann.

2. Creating a UDF Python project.

    The following uses a project whose working directory is “test\_sub” as an example to describe how to create a project.

    Run the following command in any directory.

    ```shell
    python3.12 -m dataflow.tools.create_func_ws -f sub:i0:i1:o0 -w ./test_sub -c Sub
    ```

    In the current directory, the following “test\_sub” folder is generated, and the directory structure is as follows.

    ```sh
    ├── CMakeLists.txt  // CMakeLists build configuration file. Generally, you do not need to modify it. You can modify it if special processing is required during packaging, for example, adding a configuration file for release during packaging.
    ├── func_sub.json // UDF configuration file, used to specify the input and output of the UDF. Generally, you do not need to modify it. If the generated workspace directory is inconsistent with the actual runtime, modify the workspace parameter in the file to the actual runtime path.
    ├── src_cpp
    │   └── func_sub.cpp  // Completes UDF registration and the logic of the UDF calling Python from C++. You do not need to modify it.
    └── src_python
        └── func_sub.py  // User implementation function, which you need to write yourself. You can add multiple Python files in the current directory, and they will be packaged into the runtime environment together during packaging.
    ```

    The meaning of each parameter in the command is as follows.

    - -m: Required parameter, a common Python command-line parameter, used as the script execution library module. In this tool, it is fixed to dataflow.tools.create\_func\_ws.
    - -f: Required parameter, used to input function information, input indexes, and output indexes. The format is "function_name:ix:ix:ox:ox", where i indicates input followed by the input index, and o indicates output followed by the output index. Multiple functions are separated by half-width commas. For example, sub:i0:i1:o0. In this example, the function name is sub (which is directly used as the Python function name; lowercase with underscores is recommended, and the generated cpp code concatenates "Proc" in upper camel case as the cpp function). There are two inputs, i0 and i1, and one output, o0.
    - -w: Optional parameter, the working directory. For example, ./test\_sub. In this example, the working directory is specified as “test\_sub” in the current directory. The default value is the current directory "./". Note: The generated files will directly overwrite files with the same name in the workspace.
    - -c: Optional parameter, the function class name. For example, Sub\( is directly used as the Python and C++ class name. Upper camel case style \) is recommended. If it is empty, it is generated by concatenating the function name in the -f parameter.

For more usage parameters of this tool, run "-h" to view them.
