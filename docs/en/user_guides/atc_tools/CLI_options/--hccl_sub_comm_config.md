# --hccl\_sub\_comm\_config

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:06:41.435Z pushedAt=2026-10-27T09:21:23.313Z -->

## Product Support

<!-- npu="950" id7 -->
- Ascend 950PR & 950DT products: Not supported
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
<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--hccl_sub_comm_config_res.md#id1 -->

## Function Description

Sets the path of the HCCL **sub-communication domain** configuration file, which is used to configure HCCL sub-communication parameters. This option is used in multi-card distributed training scenarios.

## Related Parameters

None

## Parameters

**Parameter value:**
Path and file name of the logical topology file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraint:**
The content in the configuration file must be in JSON format.

## Recommended Configuration and Benefits

None.

## Example

Upload the configuration file (for example, named sub\_comm\_config.json) to the server where the ATC tool is located, for example, to $HOME/conf. The following is an example:

```bash
atc --model=xxx.air --framework=1 --soc_version=<soc_version> --output=$HOME/out --cluster_config=$HOME/conf/cluster_config.json  --hccl_sub_comm_config=$HOME/conf/sub_comm_config.json
```

The following is an example of the logical topology file:

```json
{
  "GroupList": [
    {
      "RankIds": [0,1],
      "HcclCommconfig": {
        "hcclCommName": "test_group"
      }
    }
  ]
}
```

Where:

- RankIds: unique identifier of a rank. Configure it as an integer starting from 0, and ensure it is globally unique. Value range: \[0, total number of Devices -1\].

    **It is recommended that rank\_id be sorted in the order of the physical connections of Devices, that is, arrange Devices that are physically closer together. Otherwise, performance may be affected.**

    For example, if device\_ip is set in ascending order of physical connections, it is recommended that rank\_id also be set in ascending order.

- hcclCommName: name of the communication group, used by programs for calling and distinguishing.

## Usage Constraints

If a model operator contains sub-communication domain parameters but the --hccl\_sub\_comm\_config parameter is not specified during model conversion, the system executes based on the actual service configuration of the cluster by default.
