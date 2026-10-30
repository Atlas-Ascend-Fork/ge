# --cluster\_config

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:48:55.750Z pushedAt=2026-10-23T06:32:19.182Z -->

## Product Support

<!-- npu="950" id7 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id7 -->
<!-- npu="A3" id6 -->
- Atlas A3 products: Not supported
<!-- end id6 -->
<!-- npu="910b" id5 -->
- Atlas A2 products: Not supported
<!-- end id5 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id3 -->
- Atlas inference products: Not supported
<!-- end id3 -->
<!-- npu="910" id2 -->
- Atlas training products: Not supported
<!-- end id2 -->
<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--cluster_config_res.md#id1 -->

## Function Description

Sets the path of the **cluster** configuration file, which is used to configure the target execution logical device information to generate HCCL tasks. This parameter is used in multi-card distributed training scenarios.

If the model contains communication operators (currently only the AllGather communication operator is supported), this parameter is required.

## Related Parameters

None.

## Parameter Values

**Parameter value:**
Path and file name of the logical topology file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraints:**
The content in the configuration file must be in JSON format.

## Recommended Configuration and Benefits

None.

## Example

Upload the configuration file (for example, the file name is cluster\_config.json) to the server where the ATC tool is located, for example, to $HOME/conf. The usage example is as follows:

```bash
atc --model=xxx.air --framework=1 --soc_version=<soc_version> --output=$HOME/out --cluster_config=$HOME/conf/cluster_config.json ...
```

The following is an example of the logical topology file:

```json
{
    "RankTable": {
        "status": "completed",
        "version": "1.2",
        "server_count": "2",
        "server_list": [
            {
                "server_id": "node_0",
                "device": [
                    {
                        "device_id": "0",
                        "rank_id": "0"
                    }
                ]
            },
            {
                "server_id": "node_1",
                "device": [
                    {
                        "device_id": "0",
                        "rank_id": "1"
                    }
                ]
            }
        ]
    },
    "HcclCommConfig": {
        "hcclCommName": "global name"
    }
}
```

The parameters are described as follows:

- RankTable: the core of the configuration, which defines the nodes and rank information involved in the computation.
  - status: mandatory, the availability flag of the rank table.
    - completed: indicates that the rank table is available.
    - initializing: indicates that the rank table is unavailable.

  - version: mandatory, the version information of the rank table template. Set it to 1.2.
  - server\_count: optional, the number of AI Servers participating in collective communication.
  - server\_list: Required. List of AI Servers participating in collective communication.
    - server\_id: Required. AI Server identifier, string type, no more than 64 characters in length. Ensure that it is globally unique. Configuration example: node\_0.
    - device: Required. Device list.
      - device\_id: Required. Physical ID of the AI processor, that is, the serial number of the Device on the AI Server.
      - rank\_id: Required. Unique rank identifier. Configure it as an integer starting from 0, and ensure that it is globally unique. Value range: \[0, total number of Devices -1\].

- HcclCommConfig: Global configuration information of the HCCL communication layer.
  - hcclCommName: Required. Name or identifier of the global communication group.

## Dependencies and Constraints

None.
