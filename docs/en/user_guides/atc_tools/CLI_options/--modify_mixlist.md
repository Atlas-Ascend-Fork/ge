# --modify\_mixlist

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-16T14:15:01.068Z pushedAt=2026-11-17T11:19:10.006Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

In mixed precision scenarios, **modify** the mixed precision blocklist, trustlist, and gray list used by operators to specify which operators are allowed to reduce precision and which are not.

The blocklist, trustlist, and gray list can be viewed from the flag parameter value under the "precision\_reduce" parameter in the built-in optimization policy file `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/config/xxx/aic-xxx-ops-info-*.json`: (where $\{INSTALL\_DIR\} should be replaced with the file storage path after CANN software installation. Taking the root user installation as an example, the default file storage path after installation is /usr/local/Ascend/cann. *xxx* should be selected based on the actual product.)

- If the value is true (trustlist), it indicates that in mixed precision mode, precision reduction is **allowed**.
- If the value is false (blocklist), it indicates that in mixed precision mode, precision reduction is **not allowed**.
- If this parameter is not configured (gray list), it indicates that in mixed precision mode, the mixed precision processing mechanism of the current operator remains consistent with that of the previous operator. That is, if the previous operator supports precision reduction, the current operator also supports precision reduction; if the previous operator does not allow precision reduction, the current operator does not support precision reduction either.

## Related Parameters

Methods for enabling mixed precision:

- [--precision\_mode](--precision_mode.md) parameter is set to allow\_mix\_precision, allow\_mix\_precision\_fp16, allow\_mix\_precision\_bf16.
- [--precision\_mode\_v2](--precision_mode_v2.md) parameter is set to mixed\_float16, mixed\_bfloat16, mixed\_hif8.

    It cannot be configured together with the [--precision\_mode](--precision_mode.md) parameter. It is recommended to use [--precision\_mode\_v2](--precision_mode_v2.md).

    <!-- npu="A3,910b,310b" id5 -->
    bf16 is supported only on the following product models:
    <!-- end id5 -->

    <!-- npu="A3" id1 -->
    Atlas A3 products
    <!-- end id1 -->

    <!-- npu="910b" id2 -->
    Atlas A2 products
    <!-- end id2 -->

    <!-- npu="310b" id3 -->
    Atlas 200I/500 A2 inference product
    <!-- end id3 -->

    <!-- npu="950" id4 -->
    hif8 is supported only on the Ascend 950PR&950DT products.
    <!-- end id4 -->

    <!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--modify_mixlist_res.md#id1 -->

## Parameter Values

**Parameter Value:**
Path and file name of the mixed precision list.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraints**:

- The list is in \*.json format. The operator list in the file is specified by the user, and multiple operators are separated by English commas.
- The configured operator type must be the OpType of the operator defined based on Ascend IR. For details about how to determine the correspondence between operators in the original framework network model and operators supported by the AI processor, see [How Do I Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by the AI Processor](../FAQ/operator_correspondence_guide.md).

## Recommended Configuration and Benefits

None.

## Example

The following shows an example of querying the blocklist, trustlist, and gray list. A flag parameter value of true indicates the trustlist, false indicates the blocklist, and no flag parameter indicates the gray list:

```json
"Conv2D":{
    ......
    "precision_reduce":{
        "flag":"true"
     },
    ......
}
```

The following shows an example of the mixed precision list, where _ops\_info.json_ is an example file name, and _OpTypeA_, _OpTypeB_, _OpTypeC_, and _OpTypeD_ are example operators.

```json
{
  "black-list": {                  // blocklist
     "to-remove": [                // Convert the blocklist operator to a graylist operator. When configuring this parameter, ensure that the operator to be converted already exists in the blocklist
     "OpTypeA"
     ],
     "to-add": [                   // Convert the trustlist or graylist operator to a blocklist operator
     "OpTypeB"
     ]
  },
  "white-list": {                  // trustlist
     "to-remove": [                // Convert the trustlist operator to a graylist operator. When configuring this parameter, ensure that the operator to be converted already exists in the trustlist
     "OpTypeC"
     ],
     "to-add": [                   // Convert the blocklist or graylist operator to a trustlist operator
     "OpTypeD"
     ]
  }
}
```

- Assume that operator A is in the trustlist by default. If you want to configure this operator as a blocklist operator, the configuration example and system processing logic are as follows:
    1. Add the operator to the blocklist:

        ```json
        {
          "black-list": {
             "to-add": ["A"]
          }
        }
        ```

        The system removes the operator from the trustlist and adds it to the blocklist, so the operator ends up in the blocklist.

    2. Remove the operator from the trustlist and add it to the blocklist at the same time:

        ```json
        {
          "black-list": {
             "to-add": ["A"]
          },
          "white-list": {
             "to-remove": ["A"]
          }
        }
        ```

        The system removes the operator from the trustlist and adds it to the blocklist, so the operator ends up in the blocklist.

- For the scenario where an operator is only removed from the blocklist/trustlist without being added to the trustlist/blocklist, the system adds the operator to the gray list. The configuration example is as follows (for example, removing an operator from the trustlist):

    ```json
    {
      "white-list": {
         "to-remove": ["A"]
      }
    }
    ```

    The system removes the operator from the trustlist and then adds it to the gray list, so the operator ends up in the gray list.

Upload the configured _ops\_info.json_ file to any directory on the server where the ATC tool is located, for example, to $HOME/module. The usage example is as follows:

```bash
atc --precision_mode=allow_mix_precision  --modify_mixlist=$HOME/module/ops_info.json ...
```

## Dependency Constraints

None.
