# Appendix

<!-- md-trans-meta sourceCommit=f2e33566b0420150e0470a13f9852d17f1cfd42b translatedAt=2026-11-16T09:01:17.197Z pushedAt=2026-11-17T03:39:12.001Z nl=2 -->

## DataFlow Framework Introduction

**DataFlow Framework Structure**

**Figure 1** DataFlow framework structure
![](figures/dataflow_framework_architecture.png "DataFlow framework structure")

The DataFlow structure can be divided into three layers. Each layer is introduced as follows.

- DataFlow API: responsible for heterogeneous computing stream expression. App developers use the DataFlow API to express the data stream of the business and generate a computation graph based on Graph IR. For a detailed description of data stream expression, see "Data Stream Expression".
- Heterogeneous parallel compiler: responsible for heterogeneous computation graph compilation. It performs graph partitioning, graph optimization, and graph deployment on the computation graph based on the composition relationship and connection topology of heterogeneous computing units, and generates the execution Bin of each heterogeneous computing unit.
- Heterogeneous Runtime: schedules the execution Bin to the corresponding heterogeneous execution unit for execution according to the result output by heterogeneous computation graph compilation. For a detailed description of the Heterogeneous Runtime, see "Heterogeneous Runtime".

**Data Stream Expression**

The rules and restrictions of data stream graph expression are as follows.

- FlowGraph is a DAG (Directed Acyclic Graph). The data stream is directed and cyclic expression is not allowed.
- A FlowGraph without input or output is not supported.
- The activation transferred between nodes may be shared by multiple nodes and must remain immutable. Therefore, computation nodes are not allowed to modify the input activation, and copy processing is required when necessary.
- The data stream supports one-to-many, which means that the same activation is copied and then distributed to multiple nodes. However, multiple outputs of the same node are sent to the same input.
      ![](figures/260109170920368.png)

- Calling GraphProcessPoint for execution is supported to implement control flow expression such as loop iteration. The branch control logic is as follows.
      ![](figures/250530151714622.png)

**Heterogeneous Runtime**

The Heterogeneous Runtime provides different executors for CPU and NPU heterogeneous nodes, including the following.

- CPU graph executor: deployed on the CPU node and used to execute the computation graph.
- NPU graph executor: deployed on the NPU and used to execute the computation graph.
- UDF executor: a custom Func execution engine that can be flexibly deployed on the CPU node or use the AICPU on the NPU.

Each executor is deployed as an independent process. The following data communication modes are used between execution processes to exchange data: shared buffer and shared queue are used within the same node, and RDMA/UB high-performance communication is used across nodes.

The production and consumption relationship, one-to-many distribution relationship, and cross-node data communication of the data stream between executors are uniformly proxied and forwarded by FlowGW (data stream gateway).

![](figures/260109171126678.png)

## Mbuf Multiple Buffers

**Feature Overview**

Mbuf is a type of shared buffer that contains two parts of data: MbufHead and MbufData, as shown in Table 1.

**Table 1** Mbuf Introduction

|Data Type|Description|
|--|--|
|MbufHead|The data header, which contains the following types of data: basic data content, such as DataType/DataSize/Shape and other information. Key information involved in data transfer: framework-related information such as retCode/routeLabel. Other user-configurable fields: starttime/endtime/userdata, etc.|
|MbufData|A segment of buffer on Mbuf that stores the actual valid data content.|

In the buffer allocation scenario of DataFlow, Mbuf is the unified mechanism of the driver for buffer management. All FlowMsgs are generated and used in the Mbuf pool. Each FlowMsg applies for a segment of Mbuf buffer:

```sh
sizeMbuf  =
       sizeof(MbufData) // Actual buffer size contained in FlowMsg
     + sizeof(MbufHead) // MbufHead size, actually 1024B
```

> [!NOTE]
>FlowMsg is the carrier for transferring information between nodes on the FlowGraph, and is used to process operations related to FlowFunc input and output. FlowMsg uses the shared buffer Mbuf to store information.

**Background**

In the scenario where a large number of UDF processes exist and each UDF process may allocate a large amount of small buffers, it is recommended to use Mbuf multiple buffer pools. An example is provided as follows.

Assume that the total size of the Mbuf buffer pool is 10 GB. When Mbuf multiple buffer pools are not used, contiguous large buffers cannot be allocated due to buffer fragmentation. The detailed information is as follows.

![](figures/250311111739664.png)

1. At time point 1, multiple large buffers (3 GB) and multiple small buffers (2 MB) are allocated.
2. At time point 2, the large buffers are no longer used and are released to the buffer pool, becoming idle. At this point, from the overall status of the buffer pool, the overall buffer usage is: 3 GB (idle) + 2 MB (already allocated) + 3 GB (idle) + 2 MB (already allocated) + 3 GB (idle) + 2 MB (already allocated), and the available buffer is 9 GB+.
3. At time point 3, although the available buffer is 9 GB+, the allocation of 5 GB contiguous buffer fails due to buffer fragmentation (2 MB buffer blocks).

**Usage Example**

For the preceding scenario, additional buffer pool configuration is required to handle the buffer fragmentation issue. The main method is to set Mbuf as a configurable multiple buffer pool system, and place small buffers into a dedicated small buffer pool.

The following figure shows an example of Mbuf multiple buffer pools.

![](figures/250311111739664-0.png)

1. At time point 1, multiple large buffers (3 GB) and multiple small buffers (2 MB) are allocated.
2. At time point 2, the large buffers are no longer used and are released to the buffer pool, becoming idle. At this point, from the overall status of the buffer pool, the overall buffer usage is: 3 GB (idle) + 3 GB (idle) + 3 GB (idle) + 2 MB (already allocated) + 2 MB (already allocated). The available buffer in buffer pool 1 is 9 GB+, and the available buffer in buffer pool 2 is 1 GB+.

3. At time point 3, the available contiguous buffer in buffer pool 1 is 9 GB+, and the allocation of 5 GB buffer succeeds.

This solution is implemented by configuring "ge.flowGraphMemMaxSize". For a configuration example, see Example 2 in Table 2.

**Table 2** Configuration Example

|Example Name|Example Description|Example Code|
|--|--|--|
|Example 1|Allocate one buffer pool with a size of 10 GB, with no size limit on the allocated buffer.|{"ge.flowGraphMemMaxSize", "10737418240"}|
|Example 2|Allocate two buffer pools.<br>- Buffer pool 1 has a size of 10 GB, with no size limit on the allocated buffer.<br>- Buffer pool 2 has a size of 2 GB, and a maximum of 2 MB can be allocated.<br>When the allocated buffer is <= 2 MB, it is preferentially allocated in buffer pool 2. When the allocated buffer is > 2 MB, it can only be allocated in buffer pool 1.|{"ge.flowGraphMemMaxSize", "10737418240,2147483648:2097152"}|
|Example 3|Allocate three buffer pools.<br>- Buffer pool 1 has a size of 10 GB, with no size limit on the allocated buffer.<br>- Buffer pool 2 has a size of 2 GB, and a maximum of 20 MB can be allocated.<br>- Buffer pool 3 has a size of 1 GB, and a maximum of 2 MB can be allocated.<br>When allocating a buffer, the corresponding buffer pool is preferentially matched according to the allocated buffer size. For example, when the allocated buffer is <= 2 MB, it is preferentially allocated in buffer pool 3. When 2 MB < allocated buffer <= 20 MB, it is preferentially allocated in buffer pool 2. When the allocated buffer is > 20 MB, it can only be allocated in buffer pool 3.|{"ge.flowGraphMemMaxSize", "10737418240,2147483648:20971520,1073741824:2097152"}|

> [!NOTE]Note
><br>The specific meaning of ge.flowGraphMemMaxSize is as follows.
><br>In the FlowGraph scenario, this is the buffer configuration available to the network, which can be specified according to the network size. Unit: Byte. Multiple buffer pool configurations are supported, with a maximum of 128 buffer pools. The configuration format is "buffer pool 1 size:allocation limit, buffer pool 2 size:allocation limit, ..., buffer pool n size:allocation limit". If not set, a single buffer pool with a size of 10 GB is used by default, with no size limit on the allocated buffer.
><br>The value range of the buffer pool size is \[1024, 274877906944\], in Byte. The total size of all buffer pools cannot exceed 256 GB, and is subject to hardware restrictions in actual use.
><br>The value range of the allocation limit is 0 or \[1024,2147483648\], in Byte. It is an optional configuration, and the default value is 0, indicating no limit.

## numa\_config.json Configuration

numa\_config.json The meanings of all fields are as shown in Table 1.

**Table 1** Field Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|**Cluster**|-|-|-|
|cluster_nodes|Array of cluster_node. For details about cluster_node, see Table 2.|Yes|Cluster resource information description.|
|nodes_topology|nodes_topology. For details about nodes_topology, see Table 5.|No|Cross-node communication topology, such as the parameter plane communication topology between two AISERVERs.|
|**node_def: common attributes of nodes of the same type within the cluster**|-|-|-|
|node_type|String|Yes|Node type. For example, in the Atlas A2 series product scenario: ATLAS900.|
|resource_type|String|Yes|Architecture and CPU resource type on which UDF can be deployed. The value is X86 or Aarch.|
|support_links|String|Yes|Communication mode within a node, such as [HCCS,PCIE,ROCE].|
|item_type|String|Yes|Acceleration card type within a node. Example: Atlas A2 series product scenario: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, Ascend*xxx*B4. Atlas A3 series product scenario: Ascend910_*xx*.|
|inter_item_memory_access_mode|String|Yes|Address access mode between different devices within a server. Value range: NUMA or UMA.|
|h2d_bw|String|No|Bandwidth between HOST and DEVICE, such as PCIE:100Gb.|
|item_topology|item_topology. For details about item_topology, see Table 6.|No|Interconnection information between different acceleration cards within a node. In the Atlas A2 series product cloud scenario, acceleration cards are allocated in units of 1/2/4/8 cards, so if there is only one card, multi-card topology is not involved.|
|**item_def: common attributes of acceleration cards of the same type within a node**|-|-|-|
|item_type|String|Yes|Acceleration card type within a node. Example: Atlas A2 series product scenario: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, Ascend*xxx*B4.|
|memory|String|Yes|Total buffer of the entire chip. In the Atlas A2 series product scenario, configure it as [DDR:64GB]. In the Atlas A3 series product scenario, configure it as [DDR:31GB].|
|aic_type|String|Yes|Computation core type and number of cores of the acceleration card, such as [DAVINCI_V100:32].|
|resource_type|String|Yes|Ascend resource type on which UDF can be deployed. Configure it as Ascend.|

**Table 2** cluster\_node Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|node_id|Integer|Yes|Node number within the cluster. Generally, 0 is used as the primary node.|
|node_type|String|Yes|Node type. Example: Atlas A2 series product scenario: ATLAS900.|
|ipaddr|String|Yes|IP for node control plane communication, such as the HOST IP of a training server.|
|port|Integer|Yes|Port for node control plane communication.|
|data_panel|NetworkInfo|No|Data plane communication information.|
|memory|Integer|No|Available buffer on the application HOST.|
|is_local|BOOL|No|When multiple nodes form a cluster, indicates whether this node in this file is the local machine.|
|auth_lib_path|String|No|Library file that DataFlow needs to dlopen. The auth_lib_path field is written by CaaS on the cloud side. The primary and secondary scheduling processes are responsible for reading and loading the .so file and calling the related APIs.|
|deploy_res_path|String|No|Storage path of the model files. Default path for the root user: /root/runtime/deploy_res. Default path for a non-root user: ${home}/runtime/deploy_res.|
|item_list|Array of item_info. For details about item_info, see Table 4.|Yes|Acceleration cards that execute this JOB, orchestrated by cloud resource management.|

**Table 3** NetworkInfo Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|avail_ports|Array of String|No|Range of data plane communication port numbers available to this application instance on each node. The carried services are as follows. FlowGW asynchronous point-to-point communication: HOST FlowGW and Device FlowGW communicate over PCIE and do not occupy HOST OS port numbers. FlowGW communication between devices requires a unique port number to be allocated for the communication. Collective communication: send/recv point-to-point graph entry and ranktbl collective communication. FlowGW communication between multiple devices supports the RDMA protocol. The default port number allocation range between multiple devices is 16666 to 32767, and the user can customize the configuration.|

**Table 4** Item\_info Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Logical ID of the acceleration card within a node.|
|device_id|Integer|Yes|Physical ID of the acceleration card within a node.|
|ipaddr|String|No|IP address or network segment information used for communication.|

**Table 5** nodes\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|type|String|Yes|The communication library determines the communication algorithm to be selected based on the topology type, such as star.|
|protocol|String|No|Communication protocol type between nodes. The default value is RDMA. A single server does not need to configure this field. Example: RDMA:200Gb.|
|topos|Array of plane. For details about plane, see Table 8.|Yes|Interconnection plane information of acceleration cards between nodes.|

**Table 6** item\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|links_mode|String|Yes|Communication mode and bandwidth between different acceleration cards within a node, such as HCCS:128Gb.|
|links|Array of item_pair. For details about item_pair, see Table 7.|Yes|Interconnection topology between different acceleration cards within a node, such as [0, 1].|

**Table 7** item\_pair Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Acceleration card ID.|
|pair_item_id|Integer|Yes|ID of the paired acceleration card.|

**Table 8** plane Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|plane_id|Integer|Yes|The ordinal number of the communication plane between servers (provided by the cloud).|
|devices|Array of int list|Yes|List of acceleration cards belonging to this plane.|

<!-- npu="910b" id9 -->

**numa\_config.json Example in the Atlas A2 Series Product Scenario**

The numa\_config.json example is as follows. The meanings of all fields are as shown in Table 1.

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

**numa\_config.json Example in the Atlas A3 Series Product Scenario**

The numa\_config.json example is as follows. The meanings of all fields are as shown in Table 1.

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

This tool is used to create a UDF Python project template, including the corresponding folders and files. The user can develop functions based on this template, which reduces the workload of manual creation.

Storage path of this tool: \$\{install\_path\}/cann/python/site-packages/dataflow/tools/create\_func\_ws.py, where $\{install\_path\} is the installation directory of the CANN software, and the default value is "/usr/local/Ascend".

The usage is as follows.

1. Set the environment variables.

    After the CANN software is installed, when the CANN running user performs compilation and running, log in to the environment as the CANN running user and run the **source \$*\{INSTALL\_DIR\}*/set\_env.sh**** command to set the environment variables. Replace $\{INSTALL\_DIR\} with the storage path of the files after the CANN software is installed. For example, if the software is installed by the root user, the default storage path of the installed files is /usr/local/Ascend/cann.

2. Create a UDF Python project.

    The following uses creating a project with the working directory "test\_sub" as an example to describe how to create it.

    Run the following command in any path.

    ```shell
    python3.12 -m dataflow.tools.create_func_ws -f sub:i0:i1:o0 -w ./test_sub -c Sub
    ```

    In the current directory, the following "test\_sub" folder is generated, and the directory structure is as follows.

    ```sh
    ├── CMakeLists.txt  // CMakeLists compilation configuration file. Generally no user modification is required. It can be modified if special processing is required during packaging, for example, adding a configuration file for release during packaging.
    ├── func_sub.json // UDF configuration file, used to specify the input and output of the UDF. Generally no user modification is required. If the generated workspace directory is inconsistent with the actual runtime, modify the workspace parameter in the file to the actual runtime path.
    ├── src_cpp
    │   └── func_sub.cpp  // Completes UDF registration and the logic of UDF calling Python from C++. No user modification is required.
    └── src_python
        └── func_sub.py  // User implementation function, which needs to be written by the user. Multiple Python files can be added in the current directory, and they are packaged into the runtime environment together during packaging.
    ```

    The meanings of the parameters in the command are as follows.

    - -m: required parameter, a general Python command-line parameter, used as the script running library module. In this tool, it is fixed as dataflow.tools.create\_func\_ws.
    - -f: required parameter, inputs the function information, input and output indexes, in the format of "function name:ix:ix:ox:ox". i indicates input, followed by the input index. o indicates output, followed by the output index. Multiple functions are separated by half-width commas. For example, sub:i0:i1:o0. In this example, the function name is sub (which is directly used as the Python function name; lowercase with underscores is recommended, and the generated cpp code concatenates "Proc" in UpperCamelCase as the cpp function), with two inputs i0 and i1 and one output o0.
    - -w: optional parameter, working directory. For example, ./test\_sub. In this example, the specified working directory is "test\_sub" in the current directory, and the default value is the current directory "./". Note: The generated files directly overwrite files with the same name in the workspace.
    - -c: optional parameter, function class name. For example, Sub\( is directly used as the Python and C++ class name. The UpperCamelCase style \) is recommended. When it is empty, it is generated by concatenating the function name in the -f parameter.

For more usage parameters of this tool, run "-h" to view them.
