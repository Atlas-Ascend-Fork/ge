# Sample Guide

## Description

This sample demonstrates a custom pass that marks the Conv2D/Conv2DV2/ExtendConv2D+Concat pattern for zero-copy optimization. After the Conv2D/Conv2DV2/ExtendConv2D+Concat structure is identified, the Concat node is marked as a virtual operator (no hardware task generated, output reusing input memory) by setting attributes, and the layout information of each conv output in the continuous output memory of Concat is described. The pass traverses all nodes in the graph (including subgraph nodes) via GetAllNodes.

The matched fixed pattern:

```
   a         b
    \       /
    Conv2D  ExtendConv2D  ==>   conv0_out + conv1_out laid out continuously in order
       \      /                  Concat is marked as a virtual operator: no task, output reuses input memory
       ConcatD
```

Attributes set by this pass:

| Node | Attribute | Type | Description |
|------|-----------|------|-------------|
| ConcatD/ConcatV2D | `_no_task` (ATTR_NAME_NOTASK) | bool | Marks that the operator does not generate a hardware task |
| ConcatD/ConcatV2D | `_output_reuse_input` (ATTR_NAME_OUTPUT_REUSE_INPUT) | bool | Marks that the output reuses input memory |
| ConcatD/ConcatV2D | `_no_padding_continuous_input` (ATTR_NAME_NOPADDING_CONTINUOUS_INPUT) | bool | Marks that the inputs are continuous memory without padding |
| ConcatD/ConcatV2D | `_reuse_input_on_dim_index` (ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX) | int64 | The dim index for output reusing input memory (in this sample, the real concat dim obtained by comparing input and output shapes) |
| Conv2D/Conv2DV2/ExtendConv2D | `_output_offset_list_for_continuous` (ATTR_NAME_OUTPUT_OFFSET_LIST_FOR_CONTINUOUS) | list\<int64\> | Byte offset of each output in the continuous output memory of the concat, relative to the first conv output |
| Conv2D/Conv2DV2/ExtendConv2D | `total_shape` (private attr) | list\<int64\> | The complete shape of the concat output where the conv output resides |
| Conv2D/Conv2DV2/ExtendConv2D | `valid_shape` (private attr) | list\<int64\> | The valid shape of the conv output itself |

Matching conditions:

- All inputs of ConcatD/ConcatV2D come from different Conv2D/Conv2DV2/ExtendConv2D nodes, and each conv output is consumed only by this Concat;
- The format of each conv output is NCHW or NHWC, and the C dim is 32-byte aligned (the C dim size = the C axis size × the dtype size, which is a multiple of 32 bytes; in NCHW the C axis is dim 1, and in NHWC the C axis is the innermost dim 3), ensuring that the start address of each input in the continuous output memory of the concat is 32B aligned (stronger than the tensor-size 32B alignment check of the built-in GE `CheckPhonyConcatInputIs32Align`);
- The concat dim of the Concat corresponds to the C axis of the conv outputs, negative indices (counting from the last dim) are supported: dim 1 or -3 for NCHW, dim 3 or -1 for NHWC;
- Input and output shapes are static, and data types are common fixed-length types.

> Note: This sample only demonstrates how to set attributes, and does not cover the complete check chain of the built-in GE Concat no-task optimization (such as 32-byte alignment, predecessor node types, cascaded continuous input, etc.). In actual use, memory checks are required to ensure that the output addresses of each conv satisfy the continuity requirement; otherwise, runtime errors may occur.

## Directory Structure

```
├── src
│   ├──conv_concat_notask_pass.cpp  // pass implementation file
├── CMakeLists.txt                  // build script
├── data
│   ├── gen_onnx.py                 // ONNX model generation script (Conv2D+Concat structure)
│   └── run_atc.sh                  // ATC conversion script (with DUMP_GE_GRAPH)
```

## Requirements

- Compiler: GCC >= 7.3.x
- Python and dependencies: python>=3.9, onnx
- The [environment preparation](../../../../../docs/zh/build.md) has been completed.

## Build

Assume that the CANN package is installed in INSTALL_PATH, for example, `/home/HwHiAiUser/Ascend/`.

1. Set environment variables.

   ```
   source ${ASCEND_PATH}/set_env.sh
   ```

   `${ASCEND_PATH}` is the cann path under the CANN package installation directory, for example, `${INSTALL_PATH}/cann`.

2. Modify the following information in **CMakeLists.txt** as required.

    - ASCEND_PATH: the default package path. If `$ASCEND_HOME_PATH` is set by set_env.sh, no modification is required.

    - FUSION_PASS_DIR: the installation directory name of the custom fusion pass library. The default value is `fusion_passes`.

3. Run the following commands to build. After the build is complete, **libconv_concat_notask_pass.so** is generated in the **build** directory.

   ```
   mkdir build && cd build
   cmake .. && make
   ```

4. After the build succeeds, run make install to install libconv_concat_notask_pass.so to the custom fusion pass directory.

   ```
   make install
   ```

   After the sample is verified, run the following command to clean up the custom pass .so installed under the CANN package to avoid affecting subsequent UT/ST:

   ```
   make clean_custom_pass
   ```

## Run

1. Set environment variables (skip if done).

   ```
   source ${ASCEND_PATH}/set_env.sh
   ```

2. Use ATC offline inference.

    - Run the model generation script in the **data** directory to generate an ONNX model containing the Conv2D+Concat structure:

      **python3 gen_onnx.py**

      After the command is executed, the **model.onnx** model file is generated in the **data** directory.

    - Run the ATC command. Fill in soc_version according to the actual running environment (this pass takes effect only on mc62 chips, that is, soc_version starts with MC62):

      **atc --model=./model.onnx --framework=5 --soc_version=xxx --output=./model**

      Alternatively, directly use the basic run script in the **data** directory (the script internally sets environment variables such as DUMP_GE_GRAPH and performs the ATC conversion):

      ```
      bash run_atc.sh MC62
      ```

    - Check the result:

      - The following logs are displayed:

        ```
        ConvConcatNotaskPass begin.
        ConvConcatNotaskPass end.
        ```

      - This pass checks the chip version at the entry. Only mc62 chips execute the subsequent marking process; on other chips the pass is skipped directly, no attribute marking content appears in the logs, and only the following is printed:

        ```
        ConvConcatNotaskPass begin.
        Soc version is not MC62, skip.
        ConvConcatNotaskPass end.
        ```

      - Compare the intermediate dump graphs (see CANN environment variable description > compilation related > DUMP_GE_GRAPH): the Concat node carries attributes such as `_no_task`, and each Conv2D node carries attributes such as `_output_offset_list_for_continuous`.
