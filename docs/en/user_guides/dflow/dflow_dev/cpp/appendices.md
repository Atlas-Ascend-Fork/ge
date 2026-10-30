# Appendix

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-28T12:58:11.412Z pushedAt=2026-10-29T05:53:14.424Z nl=2 -->

## numa\_config.json Configuration

numa\_config.json The meaning of the full field is as shown in Table 1.

**Table 1** Field Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|**Cluster**|-|-|-|
|cluster_nodes|Array of cluster_node. For details about cluster_node, see Table 2.|Yes|Description of cluster resource information.|
|nodes_topology|nodes_topology. For details about nodes_topology, see Table 5.|No|Cross-node communication topology, for example, the parameter plane communication topology between two AISERVERs.|
|**node_def: common attributes of nodes of the same type in the cluster**|-|-|-|
|node_type|String|Yes|Node type. Example: ATLAS900 for the Atlas A2 products.|
|resource_type|String|Yes|Architecture and CPU resource type on which UDF can be deployed. The value is X86 or Aarch.|
|support_links|String|Yes|Communication mode within the node, for example, [HCCS,PCIE,ROCE].|
|item_type|String|Yes|Accelerator card type within the node. Example: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, and Ascend*xxx*B4 for the Atlas A2 products; Ascend910_*xx* for the Atlas A3 products.|
|inter_item_memory_access_mode|String|Yes|Address access mode between different devices in a server. The value range is NUMA or UMA.|
|h2d_bw|String|No|Bandwidth between HOST and DEVICE, for example, PCIE:100Gb.|
|item_topology|item_topology. For details about item_topology, see Table 6.|No|Interconnection information between different accelerator cards within the node. For the Atlas A2 product cloud, accelerator cards are allocated in units of 1/2/4/8 cards, so a single card does not involve multi-card topology.|
|**item_def: common attributes of accelerator cards of the same type in a node**|-|-|-|
|item_type|String|Yes|Accelerator card type within the node. Example: Ascend*xxx*B1, Ascend*xxx*B2, Ascend*xxx*B3, and Ascend*xxx*B4 for the Atlas A2 products.|
|memory|String|Yes|Total memory of the entire chip.For the Atlas A2 products, configure as [DDR:64GB].For the Atlas A3 products, configure as [DDR:31GB].|
|aic_type|String|Yes|Compute core type and number of cores of the accelerator card, for example, [DAVINCI_V100:32].|
|resource_type|String|Yes|Ascend resource type on which UDF can be deployed. Configure as Ascend.|

**Table 2** cluster\_node Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|node_id|Integer|Yes|Node number in the cluster. Generally, 0 is used as the primary node.|
|node_type|String|Yes|Node type. Example: ATLAS900 for the Atlas A2 products.|
|ipaddr|String|Yes|IP address for control plane communication of the node, for example, the HOST IP of the training server.|
|port|Integer|Yes|Port for control plane communication of the node.|
|data_panel|NetworkInfo|No|Data plane communication information.|
|memory|Integer|No|Available memory on the application HOST.|
|is_local|BOOL|No|Whether this node in this file is the local machine when multiple Nodes form a cluster.|
|auth_lib_path|String|No|Library file that DataFlow needs to dlopen. The auth_lib_path field is written by CaaS on the cloud assistant side, and the primary and secondary scheduling processes read and load the .so file and call the related APIs.|
|deploy_res_path|String|No|Path for storing model files. Default path for the root user: /root/runtime/deploy_res. Default path for non-root users: ${home}/runtime/deploy_res.|
|item_list|Array of item_info. For details about item_info, see Table 4.|Yes|Accelerator cards that execute this JOB as orchestrated by cloud resource management.|

**Table 3** NetworkInfo Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|avail_ports|Array of String|No|Range of data plane communication port numbers available to this application instance on each node. The services carried are as follows. FlowGW asynchronous point-to-point communication: communication between HOST FlowGW and Device FlowGW uses PCIE and does not occupy HOST OS port numbers; communication between Device FlowGWs requires a unique port number to be allocated for the communication. Collective communication: send/recv graph point-to-point and ranktbl collective communication. FlowGW communication between multiple Devices supports the RDMA protocol. The default port number allocation range between multiple Devices is 16666 to 32767, and users can customize the configuration.|

**Table 4** Item\_info Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Logical ID of the accelerator card in the node.|
|device_id|Integer|Yes|Physical ID of the accelerator card in the node.|
|ipaddr|String|No|IP address or network segment information used for communication.|

**Table 5**  nodes\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|type|String|Yes|The communication library determines the communication algorithm to use based on the topology type, for example, star.|
|protocol|String|No|Protocol type for inter-node communication. The default value is RDMA. This field does not need to be configured for a single server. Example: RDMA:200Gb.|
|topos|Array of plane. For detailed information about plane, see Table 8.|Yes|Interconnection plane information of accelerator cards between nodes.|

**Table 6**  item\_topology Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|links_mode|String|Yes|Communication mode and bandwidth between different accelerator cards within a node, for example, HCCS:128Gb.|
|links|Array of item_pair. For detailed information about item_pair, see Table 7.|Yes|Interconnection topology between different accelerator cards within a node, for example, [0, 1].|

**Table 7**  item\_pair Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|item_id|Integer|Yes|Accelerator card ID.|
|pair_item_id|Integer|Yes|ID of the paired accelerator card.|

**Table 8**  plane Description

|**Name**|**Type**|**Required**|**Description**|
|--|--|--|--|
|plane_id|Integer|Yes|Index of the communication plane between servers (provided by the cloud).|
|devices|Array of int list|Yes|List of accelerator cards that belong to this plane.|

<!-- npu="910b" id7 -->

**Atlas A2 products scenario numa\_config.json example**

numa\_config.json The example is as follows. The meaning of the full field is as shown in Table 1.

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

<!-- end id7 -->

<!-- npu="A3" id8 -->

**Example for the Atlas A3 products numa\_config.json**

numa\_config.json An example is as follows. The meaning of each full field is as shown in Table 1.

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

<!-- end id8 -->
