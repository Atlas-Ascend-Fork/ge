# Appendix

<!-- md-trans-meta sourceCommit=9b7a5d514e13d48484913fdff0ed5bb7733debf8 translatedAt=2026-11-16T08:49:13.482Z pushedAt=2026-11-17T03:39:11.983Z nl=2 -->

## numa\_config.json Configuration

The meanings of numa\_config.json fields are listed in Table 1.

**Table 1** Field description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|**Cluster**|-|-|-|
|cluster_nodes|Array of cluster_nodeFor detailed information about cluster_node, see Table 2.|Yes|Description of cluster resource information.|
|nodes_topology|nodes_topologyFor detailed information about nodes_topology, see Table 5.|No|Cross-node communication topology, for example, the parameter plane communication topology between two AISERVERs.|
|**node_def: common attributes of nodes of the same type within the cluster**|-|-|-|
|node_type|String|Yes|Node type. Example: in the Atlas A2 series product scenario: ATLAS900.|
|resource_type|String|Yes|Architecture and CPU resource type on which UDF can be deployed. The value is X86 or Aarch.|
|support_links|String|Yes|Communication mode within a node, for example, [HCCS,PCIE,ROCE].|
|item_type|String|Yes|Acceleration card type within a node. Example: in the Atlas A2 series product scenario: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, Ascend*xxx*B4. In the Atlas A3 series product scenario: Ascend910_*xx*|
|inter_item_memory_access_mode|String|Yes|Address access mode between different devices within a server. Value range: NUMA or UMA.|
|h2d_bw|String|No|Bandwidth between HOST and DEVICE, for example, PCIE:100Gb.|
|item_topology|item_topologyFor detailed information about item_topology, see Table 6.|No|Interconnection information between different acceleration cards within a node. In the Atlas A2 series product cloud scenario, acceleration cards are allocated as 1/2/4/8 cards. Therefore, if only one card is allocated, multi-card topology is not involved.|
|**item_def: common attributes of acceleration cards of the same type within a node**|-|-|-|
|item_type|String|Yes|Acceleration card type within a node. Example: in the Atlas A2 series product scenario: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, Ascend*xxx*B4.|
|memory|String|Yes|Total buffer of the entire chip. In the Atlas A2 series product scenario, configure it as [DDR:64GB]. In the Atlas A3 series product scenario, configure it as [DDR:31GB].|
|aic_type|String|Yes|Compute core type and number of cores of the acceleration card, for example, [DAVINCI_V100:32].|
|resource_type|String|Yes|Ascend resource type on which UDF can be deployed. Configure it as Ascend.|

**Table 2**  cluster\_node Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|node_id|Integer|Yes|Node number within the cluster. Generally, 0 is used as the primary node.|
|node_type|String|Yes|Node type. Example: in the Atlas A2 series product scenario: ATLAS900.|
|ipaddr|String|Yes|IP used for node control plane communication. For example, for a training server, it is the HOST IP.|
|port|Integer|Yes|Port used for node control plane communication.|
|data_panel|NetworkInfo|No|Activation plane communication information.|
|memory|Integer|No|Available buffer on the application HOST.|
|is_local|BOOL|No|Whether this node in this file is the local machine when multiple nodes form a cluster.|
|auth_lib_path|String|No|Library file that DataFlow needs to dlopen. The auth_lib_path field is written by CaaS on the cloud assistant side. The primary and secondary scheduling processes are responsible for reading and loading the .so file and calling the related APIs.|
|deploy_res_path|String|No|Path where model files are stored. Default path for the root user: /root/runtime/deploy_res. Default path for a non-root user: ${home}/runtime/deploy_res.|
|item_list|Array of item_infoFor detailed information about item_info, see Table 4.|Yes|Acceleration cards that execute this JOB in cloud resource management and orchestration.|

**Table 3**  NetworkInfo description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|avail_ports|Array of String|No|Range of activation plane communication port numbers available to this application instance on each node. The services carried are as follows. FlowGW asynchronous point-to-point communication: communication between HOST FlowGW and Device FlowGW uses PCIE and does not occupy HOST OS port numbers. Communication between Device FlowGWs requires a unique port number to be allocated for the communication. Collective communication: send/recv graph point-to-point and ranktbl collective communication. FlowGW communication between multiple devices supports the RDMA protocol. The default port number range allocated between multiple devices is 16666 to 32767, which can be customized by the user.|

**Table 4**  Item\_info Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Logical ID of the acceleration card within a node.|
|device_id|Integer|Yes|Physical ID of the acceleration card within a node.|
|ipaddr|String|No|IP address or network segment information used for communication.|

**Table 5**  nodes\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|type|String|Yes|The communication library determines the communication algorithm to be selected based on the topology type, for example, star.|
|protocol|String|No|Communication protocol type between nodes. The default value is RDMA. This field does not need to be configured for a single server. Example: RDMA:200Gb.|
|topos|Array of planeFor detailed information about plane, see Table 8.|Yes|Interconnection plane information of acceleration cards between nodes.|

**Table 6**  item\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|links_mode|String|Yes|Communication mode and bandwidth between different acceleration cards within a node, for example, HCCS:128Gb.|
|links|Array of item_pairFor detailed information about item_pair, see Table 7.|Yes|Interconnection topology between different acceleration cards within a node, for example, [0, 1].|

**Table 7**  item\_pair Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Acceleration card ID.|
|pair_item_id|Integer|Yes|ID of the paired acceleration card.|

**Table 8**  plane description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|plane_id|Integer|Yes|Index of the communication plane between servers (provided by the cloud).|
|devices|Array of int list|Yes|List of acceleration cards that belong to this plane.|

<!-- npu="910b" id7 -->

**numa\_config.json Example in the Atlas A2 series product scenario**

The numa\_config.json example is as follows. The meanings of all fields are listed in Table 1.

```json
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
          }
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

<!-- end id7 -->

<!-- npu="A3" id8 -->

**numa\_config.json Example in the Atlas A3 series product scenario**

The numa\_config.json example is as follows. The meanings of all fields are listed in Table 1.

```json
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
          }
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

<!-- end id8 -->
