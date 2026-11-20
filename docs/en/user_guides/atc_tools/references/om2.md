# OM2 Model Introduction

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T15:15:45.096Z pushedAt=2026-11-17T12:54:44.944Z nl=2 -->

## OM2 Overview

OM2 is an offline model format in which GE delivers compilation results in a "model as code" manner. Its file extension is `.om2`, and it is positioned as a transparent execution carrier that connects graph compilation and model deployment. Traditional OM stores model information in a private binary format, and developers usually need dedicated tools to analyze its execution behavior. At load time, the three operations of model structure deserialization, attribute decompression, and operator Task definition parsing account for nearly 30% of the total time, making them one of the main factors affecting model startup speed. The related processing also increases the size of GE at execution time, which is not conducive to fast startup and lightweight deployment in edge and device-side scenarios. To address this, OM2 generates model-specific C++ loading and execution source code and the dynamic library `lib<model_name>_om2.so` at compile time, and packages them together with model metadata, constant weights, operator binaries, and debug information into a ZIP file that can be directly decompressed and viewed. This improves model readability and debuggability while reducing the aforementioned processing overhead at load time. At runtime, the application reuses the acl Model API. The OM2 executor parses `model_meta.json`, constant configuration, operator binaries, and the model dynamic library from the archived data in memory, loads the model dynamic library through `dlopen`/`dlsym` and binds its exported C APIs to complete model loading, execution, and resource release. The model dynamic library calls Runtime APIs on the Host side to drive operator binaries to execute on the Ascend NPU. The overall relationship is shown in the following figure.

![](../figures/om2_overall_relationship.png "OM2 overall relationship")

Generating the `.om2` file completes model compilation. At runtime, the user passes the model file path to `aclmdlLoadFromFile`, or passes the complete model data to `aclmdlLoadFromMem`, and obtains `modelId` after successful loading. After preparing the input and output, the user initiates inference through `aclmdlExecute` or `aclmdlExecuteAsync`, and calls `aclmdlUnload` after use. Archive parsing and model dynamic library loading are completed by the OM2 execution path inside acl, and the application does not need to decompress the archive in advance. For the specific call flow, see [Loading and Executing the OM2 Model](#loading-and-executing-the-om2-model).

### Key Features

- **Transparent model structure**: The model structure, execution source code, weights, and operator binaries can be viewed with a general-purpose decompression tool, turning the compilation artifact from a "black box" into a model package that can be directly analyzed.
- **Improved loading performance**: Compared with OM, model loading performance is improved by 80%. Model-specific loading and execution code is generated at compile time, eliminating the overhead of Protobuf parsing and graph and Task structure restoration at load time.
- **Locatable issues**: The generated source code clearly presents the operator scheduling order, resource management, and address usage logic. Combined with model debug information, it helps developers locate precision and performance issues.
- **Small runtime dependency**: Only the minimum dependencies required for model scheduling are retained, and the dump function can be trimmed on demand, facilitating lightweight deployment in resource-constrained edge and device-side environments.
- **Reuse of acl APIs**: Existing aclmdl loading, execution, and unload APIs are reused. Regular migration only requires adjusting the model compilation parameters and load path, reducing integration costs.

### Cost of Use

- **Introduction of cross-compilation**: OM2 includes a model dynamic library executed on the Host side. When the CPU architecture of the compilation environment differs from that of the deployment environment, the cross-compilation toolchain and dependency libraries for the target architecture must be prepared, and the compilation command must be configured. For details, see [Target Environment](#target-environment).

### Differences from OM

| Comparison Item | OM | OM2 |
|--------|----|-----|
| Compilation method | ATC generates `.om` by default | ATC specifies `--mode=7`, or `OFFLINE_MODE=7` is specified when compiling an offline model |
| Model content | Private binary format, usually viewed with a dedicated tool | ZIP format, allowing direct viewing of model information, generated source code, dynamic library, and operator binaries |
| Loading method | Restores the model structure and parses task definitions at load time | Generates loading and execution code at compile time, and loads the model dynamic library and initializes resources at load time |

## User Interface

The usage process is: **compile to generate `.om2` → load the model through acl → prepare input and output → execute inference → unload the model**. The compilation entry can be ATC or IR/ES graph construction, and the loading and execution process is the same.

### Preparing the Environment

- Install a CANN software package that supports OM2 and set the environment variables. For details, see [Environment Preparation](../../../quick_install.md#1-environment-preparation).
- Prepare a convertible original model (ONNX is used as an example below) and inference input data. For a complete application, see the image classification sample in [Migration from OM and Sample Experience](#migration-from-om-and-sample-experience).
- **The target operating system and CPU architecture of the model dynamic library must be consistent with the operating environment of the executor**. By default, it is generated according to the compilation environment. For example, a model natively compiled in a Linux/x86_64 environment cannot be directly executed in a Linux/aarch64 environment. For cross-compilation instructions, see [Target Environment](#target-environment).

### Converting an Offline OM2 Model

The following provides the simple process for the two compilation entries. Select one of them to generate the model, and then call the acl APIs to load and execute it as described in [Loading and Executing the OM2 Model](#loading-and-executing-the-om2-model).

![](../figures/om2_compile_flow.png "OM2 compilation simple process")

#### Conversion Through ATC

Specify `--mode=7` to generate OM2. For example, to convert an ONNX model to OM2:

```bash
atc --mode=7 --model=./model/resnet50.onnx --framework=5 \
    --output=./model/resnet50_npu --soc_version=<soc_version>
```

Replace `<soc_version>` with the actual Ascend AI processor version, and fill in the model path according to the actual location. After the command succeeds, `./model/resnet50_npu.om2` is generated, and `--output` does not need a suffix. If the original command carries other parameters, verify the OM2 usage constraints of each parameter in [ATC Parameter Description](../CLI_options/README.md).

#### Generation Through IR/ES Graph Construction

After generating `ge::Graph` through IR/ES graph construction and initializing the compilation environment, set `OFFLINE_MODE=7` in the options passed to `aclgrphBuildModel`. After saving, obtain the `.om2` file and load and execute it as described in [Loading and Executing the OM2 Model](#loading-and-executing-the-om2-model):

```cpp
std::map<std::string, std::string> build_options = {
    {ge::ir_option::OFFLINE_MODE, "7"},
};
ge::ModelBufferData model;
auto ret = ge::aclgrphBuildModel(graph, build_options, model);
// After successful compilation, save and output ./model/resnet50_npu.om2.
ret = ge::aclgrphSaveModel("./model/resnet50_npu", model);
```

The above is a modification snippet in an existing program. Retain the original return value check and the `aclgrphBuildFinalize` call. For API descriptions, see [aclgrphBuildModel](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel.md) and [aclgrphSaveModel](../../../api/graph_engine_api/cpp/ge/aclgrphSaveModel.md).

The above compilation process finally delivers the `.om2` model package, which contains the generated model source code, dynamic library, weights, operator binaries, and metadata. The application can directly load and execute this model package as described in [Loading and Executing the OM2 Model](#loading-and-executing-the-om2-model). For the composition of the compilation artifact and its relationship with the original OM partitions, see [Compile-time Deliverable: OM2 Model Package](#compile-time-deliverable-om2-model-package).

### Loading and Executing the OM2 Model

The user can select the following loading path according to how the model runtime memory is managed. The `modelId` returned after successful loading is used for subsequent model description query, inference execution, and model unload.

![](../figures/om2_loading_and_execution.png "OM2 loading and execution")

Load the compilation artifact through the aclmdl APIs:

```cpp
const char *omModelPath = "./model/resnet50_npu.om2";
```

The call sequence for loading, execution, and unload is as follows. Only model-related calls are shown here. A complete application also needs to complete acl initialization, Device/Context setting, Dataset preparation, return value check, and resource release.

```cpp
uint32_t modelId;
aclError ret = aclmdlLoadFromFile(omModelPath, &modelId);
// After successful loading, prepare inputDs and outputDs according to the model description.
ret = aclmdlExecute(modelId, inputDs, outputDs);
ret = aclmdlUnload(modelId);
```

acl automatically identifies om and om2. There is no need to add format judgment in the application, nor to decompress the model package first. The input and output sizes are still queried through the model description APIs. Do not reuse the hardcoded memory size of the old model.

If you need to manage the model working memory and weight memory by yourself, call `aclmdlQuerySize` (file) or `aclmdlQuerySizeFromMem` (memory) to query the size before loading. After applying for Device memory, load through `aclmdlLoadFromFileWithMem` or `aclmdlLoadFromMemWithMem` respectively. You can also use `aclmdlSetConfigOpt` and `aclmdlLoadWithConfig` to configure loading. The working memory and weight memory here serve a different purpose from the input and output buffers prepared by the application.

For API usage, see [aclmdlLoadFromFile](../../../api/graph_engine_api/c/acl/aclmdlLoadFromFile.md), [aclmdlLoadFromMem](../../../api/graph_engine_api/c/acl/aclmdlLoadFromMem.md), [aclmdlExecute](../../../api/graph_engine_api/c/acl/aclmdlExecute.md), and [aclmdlExecuteAsync](../../../api/graph_engine_api/c/acl/aclmdlExecuteAsync.md).

### Migration from OM and Sample Experience

For an existing OM inference program, regular migration only requires two steps: change the ATC `--mode=0` to `--mode=7` (add this parameter if it was not specified originally), and change the load path to the generated `.om2` file. The initialization, input/output Dataset construction, model execution, and resource release processes reuse the original program. **Do not simply rename the `.om` file to `.om2`**.

Taking ATC compilation and loading the model from a file as an example, the differences in usage are as follows. The orange nodes mark the two places that need to be adjusted during migration, and the blue nodes indicate the reused original call process. The acl initialization and device setting before loading also reuse the original program.

![](../figures/om2_migration.png "OM2 migration process")

When using additional compilation parameters or load configuration items, first verify [ATC Parameter Description](../CLI_options/README.md) and [Constraints](#constraints). The migration points for different loading and execution methods are as follows:

| Original Usage | Handling When Switching to OM2 |
|----------|--------------------|
| Load from file `aclmdlLoadFromFile` | Modify the model file path |
| Load from memory `aclmdlLoadFromMem` | Read the complete `.om2` file data and pass the data address and byte count |
| Asynchronous execution `aclmdlExecuteAsync` | Reuse the original stream, execution, and synchronization process |
| Configured loading `aclmdlLoadWithConfig` | Update the model path or memory data, and verify [Load Configuration Items Supported by OM2](#load-configuration-items) |

You can experience OM2 based on the [ResNet-50 image classification sample](../../../../../examples/acl/1_sample_resnet50_imagenet_classification/README.md):

1. Prepare the environment, model, and test image according to the sample.
2. Add `--mode=7` to the model conversion command in the sample to generate `model/resnet50.om2`.
3. Change the path argument of `sampleRes50.PrepareModel("../model/resnet50.om")` in `src/sample_resnet50_imagenet_classification.cpp` to `"../model/resnet50.om2"`.
4. Complete image preprocessing, compilation, and running according to the original sample, and check `SAMPLE PASSED` and the classification result.

Except for the model conversion parameters and load path, reuse the other steps of the sample.

## Compile-time Deliverable: OM2 Model Package

[User Interface](#user-interface) describes how to generate and use the `.om2` file. This section further describes the internal structure of this compile-time deliverable and the correspondence between its parts and the original OM partitions.

### Model Package Structure

For the directory structure of the model package and the JSON field descriptions, see [OM2 Model Internal Structure](internal_structure_of_om2_model.md).

### Correspondence Between the OM2 Directory Structure and the Original OM Partitions

OM2 organizes model metadata, execution code, operator binaries, and weights into viewable directories and files. The following table compares them with the original OM partitions by responsibility. The OM2 paths use the naming in the current implementation and are all relative to the root prefix in the package.

| OM2 File or Directory | Corresponding Part in Original OM | Description |
|----------------|----------------|------|
| `manifest.json` | OM Header (model file header) | Provides a package-level manifest that records version compatibility information, the number of models, and the ATC command that generated the model |
| `data/model_N/model_meta.json` | `MODEL_INOUT_INFO` | Saves model input/output specifications, working memory size, dynamic shape ranges, and other metadata for loading and model description query |
| `data/model_N/runtime/` | No directly corresponding partition | Saves the generated model C++ source code, build files, and model dynamic library, carrying the model-specific loading, execution, and resource management logic |
| `data/model_N/debug/ge_visual_00000000_graph_0.json` | `MODEL_DEF` (debug display responsibility) | Saves visualization information such as graphs, operators, connection relationships, and attributes for analyzing compilation results. Normal model loading does not depend on this debug file to restore the graph structure |
| `data/kernels/` | `TBE_KERNELS` | Saves precompiled operator binaries for registration at model load time and invocation at execution time |
| `data/constants/` | `WEIGHTS_DATA` | Saves the binary data of built-in constants and weights. The file name, offset, and size are recorded in the `constants_config.json` of each model |

This table is a comparison of functional responsibilities, not a direct split of the OM binary partitions into files. For example, the `TASK_INFO` of the original OM is parsed at load time, whereas OM2 generates the model task scheduling code from it at compile time and delivers it with the model dynamic library in `runtime/`.

## Constraints

### Target Environment

The OM2 generated through the ATC or IR/ES graph construction entry includes a model dynamic library executed on the Host side. **The target operating system and CPU architecture of the model dynamic library must be consistent with the operating environment of the executor**. The OM2 file can be decompressed and viewed across platforms, but the dynamic library in the package cannot be directly executed across architectures.
By default, the local toolchain is used to generate the model dynamic library. For example, an OM2 obtained by native compilation in a Linux/x86_64 environment cannot be directly run in a Linux/aarch64 environment. If it needs to be deployed to Linux/aarch64, it must be cross-compiled with the corresponding toolchain.

#### Compilation Environment and Configuration Methods

The compilation environment requires an available C++ compilation toolchain and `make`. For cross-architecture compilation, the dependency libraries corresponding to the target architecture must also be prepared. The configuration methods for different compilation entries are as follows.

**ATC Entry**

When `--host_env_os=linux --host_env_cpu=aarch64` is used to specify the cross-compilation target environment, only generating a Linux/aarch64 target model on an x86_64 host is supported. Generating a Linux/x86_64 target model in reverse on an aarch64 host through this set of parameters is not supported. The command example is as follows:

```bash
atc --mode=7 --model=./model/resnet50.onnx --framework=5 \
    --output=./model/resnet50_npu --soc_version=<soc_version> \
    --host_env_os=linux --host_env_cpu=aarch64
```

Replace `<soc_version>` with the target Ascend processor version. `--host_env_cpu` can also be set to `arm64`. For parameter descriptions, see [--host_env_os](../CLI_options/--host_env_os.md) and [--host_env_cpu](../CLI_options/--host_env_cpu.md).

In addition, `--build_config` can be used to specify the make command template used when the generated source code is compiled into the model dynamic library during the graph compilation phase, for customizing parameters such as the cross compiler, compilation options, preprocessing options, and linking options. For parameter descriptions, see [--build_config](../CLI_options/--build_config.md).

**IR/ES Graph Construction Entry**

The IR/ES graph construction entry currently supports configuring the compilation command of the model dynamic library only through `OPTION_BUILD_CONFIG`, and does not support `HOST_ENV_OS` or `HOST_ENV_CPU`. Its usage is the same as the ATC `--build_config`. It configures `CXX`, `CXXFLAGS`, `CPPFLAGS`, `LDFLAGS`, and `LDLIBS` through the make command template, and there is no need to specify the Makefile path.

The following snippet shows how to specify the aarch64 cross compiler:

```cpp
std::map<std::string, std::string> build_options = {
    {ge::ir_option::OFFLINE_MODE, "7"},
    {ge::ir_option::OPTION_BUILD_CONFIG, "make -s CXX=/usr/bin/aarch64-linux-gnu-g++"},
};

ge::ModelBufferData model;
auto ret = ge::aclgrphBuildModel(graph, build_options, model);
```

**After setting `OPTION_BUILD_CONFIG`, the default cross compiler and target library paths are not automatically supplemented**. Modify the compiler path in the example according to the actual environment, and configure the dependency libraries and compilation and linking options for the target architecture. For the template format and limitations, see [OPTION_BUILD_CONFIG](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/operator_and_graph_compilation.md#option_build_config).

### Compilation Parameters

- When generating OM2 through ATC (`--mode=7`), for the usage constraints of each compilation parameter, see [ATC Parameter Description](../CLI_options/README.md).
- When generating OM2 through the IR/ES graph construction entry (`OFFLINE_MODE=7`), for the usage constraints of each build option, see [Configuration Parameters Supported by aclgrphBuildModel](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/aclgrphbuildmodel_config_params.md).

### Load Configuration Items

For the support of load configuration items for OM2, see [aclmdlConfigAttr](../../../api/graph_engine_api/c/acl/aclmdlConfigAttr.md). For the configuration API usage, see [aclmdlSetConfigOpt](../../../api/graph_engine_api/c/acl/aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](../../../api/graph_engine_api/c/acl/aclmdlLoadWithConfig.md). If an unsupported item is detected in the configuration at load time, `ACL_ERROR_INVALID_PARAM` is returned.
