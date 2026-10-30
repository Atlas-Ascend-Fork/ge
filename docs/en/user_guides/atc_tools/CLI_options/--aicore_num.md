# --aicore\_num

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:44:20.613Z pushedAt=2026-10-22T13:10:41.515Z -->

## Product Support

Full chip support

## Function Description

Used to configure the AI Core count used during model compilation.

## Parameter Values

**Parameter value:** "integer1|integer2", separated by "|".

<!-- npu="950,A3,910b" id11 -->
- **Scenario 1**: For the following products, integer1 indicates the number of Cube Cores in the AI Core used for operator compilation, and integer2 indicates the number of Vector Cores in the AI Core used for operator compilation. Both integer1 and integer2 must be greater than or equal to 0 and less than or equal to the maximum number of Cube Cores and Vector Cores contained in the AI processor:

    <!-- npu="950" id1 -->
    Ascend 950PR&950DT products
    <!-- end id1 -->

    <!-- npu="A3" id2 -->
    Atlas A3 products
    <!-- end id2 -->

    <!-- npu="910b" id3 -->
    Atlas A2 products
    <!-- end id3 -->
<!-- end id11 -->

<!-- npu="910,310p,310b" id12 -->
- **Scenario 2**: For the following products, only integer1 needs to be configured, in the format "integer1|". Configuring integer2 does not take effect. It indicates the number of AI Cores used for operator compilation:

    <!-- npu="310b" id4 -->
    Atlas 200I/500 A2 inference product
    <!-- end id4 -->

    <!-- npu="310p" id5 -->
    Atlas inference products
    <!-- end id5 -->

    <!-- npu="910" id6 -->
    Atlas training products
    <!-- end id6 -->

<!-- end id12 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--aicore_num_res.md#id1 -->

**Parameter value constraints**:

<!-- npu="950,A3,910b" id13 -->
- For scenario 1 in the parameter values:

    The maximum number of Cube Cores and Vector Cores contained in different AI processors can be viewed from the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file, as shown below, indicating that the AI processor has 24 Cube Cores and 48 Vector Cores.

    ```ini
    [SoCInfo]
    # The parameter is configured with the default value, which is the maximum value
    ai_core_cnt=24
    cube_core_cnt=24
    vector_core_cnt=48
    ```
<!-- end id13 -->

<!-- npu="910,310p,310b" id14 -->
- For scenario 2 in the parameter values:

    The maximum number of AI Cores contained in different AI processors can be viewed from the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file, as shown below, indicating that the AI processor has 10 AI Cores.

    ```ini
    [SoCInfo]
    # Default value of AI Core, which is the maximum value
    ai_core_cnt=10
    vector_core_cnt=8
    ```
<!-- end id14 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--aicore_num_res.md#id2 -->
- Configuring the value as 0 does not cause an error, but it is treated as unconfigured and does not produce a core control effect. For example, configuring "integer1|0" is equivalent to configuring "integer1|".
- If the operator compilation cache function is enabled while this parameter is configured (the [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md) parameter is set to "enable" or "force"), this parameter takes effect only during the first compilation. If you want this parameter to take effect in non-first compilations, you need to clear the cache on the compilation disk.

Here, $\{INSTALL\_DIR\} must be replaced with the file storage path after CANN software installation. Taking installation by the root user as an example, the default file storage path after installation is /usr/local/Ascend/cann. <arch\> indicates the specific operating system architecture, and xxx must be selected based on the actual product.

## Recommended Configuration and Benefits

None.

## Example

<!-- npu="950,A3,910b" id15 -->
- Scenario 1 configuration example:

    ```bash
    atc --aicore_num="24|48" ...
    ```
<!-- end id15 -->

<!-- npu="910,310p,310b" id16 -->
- Scenario 2 configuration example

    ```bash
    atc --aicore_num="10|" ...
    or
    atc --aicore_num="10" ...
    ```
<!-- end id16 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--aicore_num_res.md#id3 -->

## Understanding the Relationship Among AI Core, Cube Core, and Vector Core

To help you understand the relationship among AI Core, Cube Core, and Vector Core, the definition of Core is clarified first. A Core is a compute core that has an independent Scalar compute unit. Typically, the Scalar compute unit is responsible for functions such as SIMD (Single Instruction Multiple Data) instruction issue of a compute core. Therefore, this Scalar compute unit is also commonly referred to as the scheduling unit within the core. The AI data processing core units differ across products and are currently classified into the following categories:

- When the AI data processing core unit is an AI Core:
  - Within an AI Core, Cube and Vector share one Scalar scheduling unit.

    <!-- npu="910" id8 -->
    Here, the Atlas training products are used as an example.
    <!-- end id8 -->

    ![Diagram](../figures/logical_architecture.png)

  - Within an AI Core, Cube and Vector each have their own Scalar scheduling unit, and are therefore referred to as Cube Core and Vector Core, respectively. In this case, one Cube Core and a group of Vector Cores are defined as one AI Core, and the number of AI Cores is usually calculated based on the number of Cube Cores.

    <!-- npu="910b" id9 -->
    Here, the Atlas A2 products are used as an example.
    <!-- end id9 -->

    ![Diagram](../figures/logical_architecture_0.png)

- When the AI data processing core units are an AI Core and a separate Vector Core: Both the AI Core and the Vector Core have independent Scalar scheduling units.

    <!-- npu="310p" id10 -->
    Here, the Atlas inference products are used as an example.
    <!-- end id10 -->

    ![Diagram](../figures/logical_architecture_1.png)

## Dependencies and Constraints

None.
