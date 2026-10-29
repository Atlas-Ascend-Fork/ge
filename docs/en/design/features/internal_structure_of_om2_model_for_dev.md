# Internal Structure of an OM2 Model Package

This document describes the files and JSON fields in an OM2 model package for developers who need to inspect or parse the package.

## 1. Package File Inventory

OM2 is a ZIP archive. All paths below are relative to the root prefix `<base_name>/` inside the package: this is usually the graph name for online graph construction and the output file name when saved by ATC. `N` is a zero-based model index. A single model uses `model_0`; a Bundle uses successive indices according to its model count.

The second column describes the model scenarios in which each file appears. "All models" means normally built static models, dynamic-gear models, and either kind with AIPP. AIPP provides input preprocessing and can be combined with static or dynamic-gear models; it is not a mutually exclusive third model category. The presence of weights, variables, custom operators, and related files also depends on the actual model data and compilation artifacts.

| Package File/Directory | Presence Condition/Applicable Scenario | Description |
|---------------|-------------------|------|
| `manifest.json` | All models | Package manifest containing version information, the model count, and the ATC command |
| `data/model_N/` | All models | Data directory for each submodel; the count is specified by `model_num` in `manifest.json` |
| `data/model_N/model_meta.json` | All models | Model metadata, including the model name, working memory, input/output Tensor descriptions, and dynamic-gear and AIPP configurations; see Section 2.2 |
| `data/model_N/op_attr.json` | All models | Operator attributes; currently records the original operator name list for each operator |
| `data/model_N/constants_config.json` | All models | Constant configuration, including the total internal weight size and the data source, file name, offset, and size of each constant |
| `data/model_N/variables_config.json` | The variable configuration object exists and `var_metas` or `entries` is nonempty | Variable configuration, including the graph ID, variable metadata, variable resources, transformation paths, and initialization data locations |
| `data/model_N/runtime/lib<model>_om2.so` | All models | Compiled model shared library containing model loading, execution, and resource release logic; exports the `GertModel*` C interfaces |
| `data/model_N/runtime/csrc/` | All models (source code always packed) | Model-specific C++ sources, interface headers, and build files for recompilation; always written by the packager, but not required for model loading (loading reads only the runtime `.so`) |
| `data/model_N/runtime/csrc/om2_model_api.h` | All models (always packed) | Copy of the runtime C API header; definitions are covered in the header reference |
| `data/model_N/runtime/csrc/<model>_internal.h` | All models (always packed) | Internal model interface declarations |
| `data/model_N/runtime/csrc/<model>_resources.cpp` | All models (always packed) | Runtime resource initialization and release logic |
| `data/model_N/runtime/csrc/<model>_args_manager.cpp` | All models (always packed) | Model argument table management logic |
| `data/model_N/runtime/csrc/<model>_kernel_reg.cpp` | All models (always packed) | Operator binary registration logic |
| `data/model_N/runtime/csrc/<model>_load_and_run.cpp` | All models (always packed) | Model loading and execution logic |
| `data/model_N/runtime/csrc/Makefile` | All models (always packed) | Build entry point for recompilation |
| `data/model_N/debug/ge_visual_00000000_graph_0.json` | The model's debug object exists | Model graph visualization JSON containing debugging information such as graphs, operators, connections, and attributes |
| `data/constants/constant_N` | The model has `internal_weight_size > 0` and internal weight data; not generated for purely external weights | Binary data for internal constants and weights, referenced by file name, offset, and size in the constant configuration |
| `data/variables/var_weight_data_N` | Variables contain nonempty initialization data | Initialization binary data concatenated by variable entry, located by offsets and sizes in the variable configuration |
| `data/kernels/` | The model contains operator binaries | Precompiled operator binaries, such as `te_{opType}_{hashId}.o`, which can be referenced by multiple models in the package |
| `data/custom_ops/binaries_npu_arch/` | The model contains custom operator binaries | Stores the corresponding custom operator binaries |
| `data/custom_ops/shared_libs/` | The model contains custom operator shared libraries | Stores the corresponding custom operator shared libraries |

## 2. JSON File Fields

Each subsection below describes one JSON file. Dots denote nested fields, `[]` denotes array elements, and `<key>` denotes a dynamic object key. Types are JSON types, not C++ memory layouts; integer bit widths, units, and serialization conditions are stated in the descriptions. `struct_size`, `reserved`, and in-process pointers are not serialized as JSON fields.

Usage descriptions reflect the current implementation. ACL interfaces usually consume fields indirectly through deserialization, the executor, or `aclmdlDesc`, rather than reading JSON directly. Tables use public ACL interface names; the corresponding OM2 implementation functions usually have the `ImplOm2` suffix. Internal interfaces are identified by class or function name. Fields that are only parsed and stored, used for diagnostics, or unused by current consumers are explicitly identified; a field name alone does not imply a corresponding ACL capability.

### 2.1 manifest.json

| JSON Field | Type | Required | Description |
|-----------|------|------|------|
| `compatibility` | object | Yes | Version and feature information object; parsed by `DeserializeManifest` and checked by `ValidateVersionCompatibility`, shared by ACL model loading and metadata query paths |
| `compatibility.compiler_version` | string | Yes | Compiler-side version; `ValidateVersionCompatibility` compares the compiler and current executor major versions and rejects parsing if the compiler major version is higher |
| `compatibility.required_executor_version` | string | Yes | Executor version required by the model; when nonempty, `ValidateVersionCompatibility` compares it with the current executor version and rejects parsing if the required version is higher |
| `compatibility.used_features` | object | Yes | Map from used feature names to version strings; an empty object if no features are used. `BuildUsedFeaturesStr` uses it for version incompatibility diagnostics; versions are not currently checked feature by feature |
| `compatibility.used_features.<feature_name>` | string | No (written when the corresponding feature is used) | Version string for the feature; included in compatibility diagnostics by `BuildUsedFeaturesStr`, with no independent ACL query interface |
| `model_num` | integer | Yes | Model count with uint64 semantics; 1 for a single model, or the submodel count for a Bundle. `PrepareModelUnit` uses it to validate submodel indices, and `GetOm2BundleInfo` uses it to enumerate submodels, indirectly serving `aclmdlBundleGetModelNum` and `aclmdlBundleGetQueryModelNum` |
| `atc_command` | string | Yes | Recorded ATC command; may be empty for non-ATC paths. Parsed and stored by `DeserializeManifest`; current OM2 ACL loading and query implementations do not read this member to return it to users |

The Required column reflects packer behavior: "Yes" means the packer always writes the field. On the deserialization side, only the top-level `compatibility` and `model_num` fields are strictly validated; all other fields are leniently skipped when missing and do not cause parsing failures.

### 2.2 model_meta.json

Corresponds to each `data/model_N/model_meta.json`. Generated at compile time and parsed into `GertModelDataModelMeta` at load time to support ACL model description and query interfaces. Resource requirements of the model SO are embedded in generated code. A missing file or model name causes deserialization to fail.

| JSON Field | Type | Required | Description |
|-----------|------|------|------|
| `name` | string | Yes | Model name, corresponding to the in-memory member `model_name`; used by `Om2ModelExecutor::Impl::InitModelDumpInfo` to populate dump/profiling model information, not as the source for input/output Tensor name queries |
| `work_size` | integer | Yes | Working memory size in bytes with uint64 semantics; returned by `aclmdlQuerySize`, `aclmdlQuerySizeFromMem`, and `aclmdlQueryWorkspaceSize` / `aclmdlQueryWorkspaceSizeFromMem` in default mode. `aclmdlBundleGetSize` returns this value for the corresponding submodel; `CheckExternalWorkSize` uses it to validate external working memory at load time |
| `zero_copy_size` | integer | Yes | Deductible byte count for input/output memory optimization, with int64 semantics; `aclmdlQueryWorkspaceSize` / `aclmdlQueryWorkspaceSizeFromMem` returns `work_size - zero_copy_size` in `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT` mode. `CheckExternalWorkSize` unconditionally validates external working memory against `work_size - zero_copy_size` at load time |
| `inputs` | `array<object>` | Yes | Input Tensor description array; populated in the description object through `aclmdlGetDesc` or `aclmdlGetDescFromFile` / `aclmdlGetDescFromMem` for `aclmdlGetNumInputs` and input query interfaces; also used to check the input count during execution |
| `outputs` | `array<object>` | Yes | Output Tensor description array; populated in the description object through model description retrieval interfaces for `aclmdlGetNumOutputs` and output query interfaces; also used to check the output count during execution |
| `inputs[].name` / `outputs[].name` | string | Yes | Tensor names; used respectively by `aclmdlGetInputNameByIndex` / `aclmdlGetOutputNameByIndex` and `aclmdlGetInputIndexByName` / `aclmdlGetOutputIndexByName`, and also by `aclmdlGetTensorRealName`. Dynamic-gear input names are also used by `aclmdlSetInputDynamicDims` to match original shapes |
| `inputs[].index` / `outputs[].index` | integer | Yes | Array index written according to the serialization position; currently not read by `ParseModelMetaInputs` / `ParseModelMetaOutputs`. ACL query indices follow the actual array order |
| `inputs[].shape` | `array<integer>` | Yes | Input shape for static models, used by `aclmdlGetInputDims`; for dynamic-gear models, the original input shape, which may contain -1. Parsed into `origin_input_dims` for `aclmdlSetInputDynamicDims` to identify dynamic axes; the regular input description is then overridden by `max_gear_shape` |
| `outputs[].shape` | `array<integer>` | Yes | Output shape; used by `aclmdlGetOutputDims` and as the fallback for `aclmdlGetCurOutputDims` when the current gear is unset. `PrepareOm2Tensor` also uses it to construct execution Tensors |
| `inputs[].max_gear_shape` | `array<integer>` | No (written only for dynamic-gear models) | Maximum-gear input shape; overrides `input_desc.shape` after parsing for `aclmdlGetInputDims` and `PrepareOm2Tensor`. Also initializes the V2 description when no dedicated V2 shape is provided |
| `inputs[].shape_aclmdlGetInputDimsV2` | `array<integer>` | No (written when `aipp_infos` is nonempty) | Input shape with `aclmdlGetInputDimsV2` semantics; falls back to the original input description when no corresponding V2 description exists. `ParseModelMetaInputs` writes it to `input_desc_v2.shape`, ultimately returned by `aclmdlGetInputDimsV2` |
| `inputs[].data_type` / `outputs[].data_type` | integer | Yes | `ge::DataType` enum value, such as `DT_FLOAT=0`; used respectively by `aclmdlGetInputDataType` / `aclmdlGetOutputDataType`, and by `PrepareOm2Tensor` to set execution Tensor types. Dynamic range size calculations also use the data type's byte size |
| `inputs[].format` / `outputs[].format` | integer | Yes | `ge::Format` enum value, such as `FORMAT_NCHW=0`; used respectively by `aclmdlGetInputFormat` / `aclmdlGetOutputFormat`, and by `PrepareOm2Tensor` to set execution Tensor formats |
| `inputs[].size` / `outputs[].size` | integer | Yes | Tensor size in bytes with uint64 semantics; the source of the return values of `aclmdlGetInputSizeByIndex` / `aclmdlGetOutputSizeByIndex`, respectively. If the shape contains negative values and the range is nonempty, these interfaces calculate the size from the maximum range instead |
| `inputs[].shape_range` / `outputs[].shape_range` | `array<array<integer>>` | Yes | `[min,max]` for each dimension; may be an empty array for static models. Input ranges serve `aclmdlGetInputDimsRange`; input and output ranges participate in maximum size calculations for `aclmdlGetInputSizeByIndex` / `aclmdlGetOutputSizeByIndex`, respectively. Input ranges also determine dynamic AIPP validation branches |
| `dynamic_dims` | object | No (written when `dynamic_batch_info` is nonempty) | Parsed by `ParseModelMetaDynamicDims`; used by `aclmdlGetDesc` to populate gear information and by dynamic-gear setting interfaces for validation |
| `dynamic_dims.dynamic_type` | integer | Yes | Dynamic-gear type, using the GE dynamic-gear type enum values; `GetDynamicTensorInfoHelp` populates batch, HW, or dims gears by type. `Om2ModelExecutor::SetDynamicSize` validates the type requested by `aclmdlSetDynamicBatchSize`, `aclmdlSetDynamicHWSize`, or `aclmdlSetInputDynamicDims` |
| `dynamic_dims.user_designate_shape_order` | `array<string>` | Yes | User-specified order of shape information; saved as `dataNameOrder` by `aclmdlGetDesc` for `aclmdlGetInputDynamicDims` to reconstruct input dimensions for each gear. `aclmdlSetInputDynamicDims` concatenates original shapes in this order and extracts dynamic axes |
| `dynamic_dims.gears` | `array<object>` | Yes | Gear array; its length serves gear queries such as `aclmdlGetInputDynamicGearCount`, and `Om2ModelExecutor::GetBatchInfoSize` uses it to provide `shapeCount` for `aclmdlGetFirstAippInfo` |
| `dynamic_dims.gears[].inputs` | `array<integer>` | Yes | Dynamic dimension values for the gear, corresponding to one row of `dynamic_batch_info`; queried by `aclmdlGetDynamicBatch`, `aclmdlGetDynamicHW`, and `aclmdlGetInputDynamicDims`, and used by `SetDynamicSize` to validate gear setting requests. `aclmdlGetCurOutputDims` also uses it to locate the current gear |
| `dynamic_dims.gears[].outputs` | `array<array<integer>>` | Yes | Output shapes for the gear; converted from in-memory comma-separated strings grouped by gear and output, with dimensions taken from the third item onward. Populated in the description object through `GetModelAttrs` and `GetModelOutputShapeInfoHelp` for `aclmdlGetCurOutputDims` to return current-gear output dimensions |
| `aipp` | object | No (written when `aipp_infos` is nonempty) | The array retains only non-null entries with `aipp_type != DATA_WITHOUT_AIPP`; parsed by `ParseAippJson` for AIPP queries and model constraint validation when setting AIPP |
| `aipp.aipp_infos` | `array<object>` | Yes | AIPP input configuration array; saved by `Om2ModelExecutor` for indirect use by `aclmdlGetAippType`, `aclmdlGetFirstAippInfo`, `aclmdlSetInputAIPP`, and `aclmdlSetAIPPByInputIndex` |
| `aipp.aipp_infos[].index` | integer | Yes | Index in the original AIPP array; `ParseAippJson` restores sparse configuration positions using this value so that `GetAippInfo`, `GetAippType`, and related interfaces can look up entries by ACL input index |
| `aipp.aipp_infos[].aipp_type` | integer | Yes | `ge::InputAippType` enum value; returned by `aclmdlGetAippType` and used by `aclmdlSetInputAIPP` / `aclmdlSetAIPPByInputIndex` to check dynamic AIPP input associations. Other executor AIPP queries also use it to determine whether a configuration exists |
| `aipp.aipp_infos[].aipp_data_index` | integer | Yes | Associated input index, or `kOm2InvalidAippDataIndex` (0xFFFFFFFF) if none; `aclmdlGetAippType` returns this associated index for dynamic AIPP, and `aclmdlSetAIPPByInputIndex` uses it to locate the parameter input |
| `aipp.aipp_infos[].aipp_mode` | integer | Yes | AIPP mode; parsed into `AippConfigInfo` by `ParseAippConfigFromJson` and available through internal `GetAippInfo`. Currently not mapped to ACL query results by `SetAippInfo` or read by dynamic AIPP validation |
| `aipp.aipp_infos[].input_format` | integer | Yes | AIPP input format; returned as `inputFormat` by `aclmdlGetFirstAippInfo` through `GetAippInfo` and `SetAippInfo` |
| `aipp.aipp_infos[].src_image_size_w` / `src_image_size_h` | integer | Yes | Source image width and height; subsequent AIPP fields without a prefix in this table belong to the same array element. Returned as `srcImageSizeW` / `srcImageSizeH` by `aclmdlGetFirstAippInfo` |
| `crop` / `resize` / `padding` / `csc_switch` | integer | Yes | Crop, resize, padding, and color space conversion switches; 0 disables, nonzero enables. `aclmdlGetFirstAippInfo` returns the corresponding switches through `SetAippInfo` |
| `load_start_pos_w` / `load_start_pos_h` | integer | Yes | Crop starting position; returned as `loadStartPosW` / `loadStartPosH` by `aclmdlGetFirstAippInfo` |
| `crop_size_w` / `crop_size_h` | integer | Yes | Crop output width and height; returned as `cropSizeW` / `cropSizeH` by `aclmdlGetFirstAippInfo` |
| `resize_output_w` / `resize_output_h` | integer | Yes | Resize output width and height; returned as `resizeOutputW` / `resizeOutputH` by `aclmdlGetFirstAippInfo` |
| `left_padding_size` / `right_padding_size` / `top_padding_size` / `bottom_padding_size` | integer | Yes | Padding on all four sides; `aclmdlGetFirstAippInfo` returns the corresponding values through `SetAippInfo` |
| `matrix_r0c0` ~ `matrix_r2c2` | integer | Yes | Nine coefficients of the 3×3 CSC matrix; returned as `matrixR0C0` ~ `matrixR2C2` by `aclmdlGetFirstAippInfo` |
| `output_bias_0` ~ `output_bias_2` / `input_bias_0` ~ `input_bias_2` | integer | Yes | Six CSC output and input biases; returned as `outputBias0` ~ `outputBias2` / `inputBias0` ~ `inputBias2` by `aclmdlGetFirstAippInfo` |
| `rbuv_swap_switch` / `ax_swap_switch` / `single_line_mode` | integer | Yes | Channel swap and single-line mode configuration; returned as `rbuvSwapSwitch` / `axSwapSwitch` / `singleLineMode` by `aclmdlGetFirstAippInfo` |
| `mean_chn_0` ~ `mean_chn_3` | integer | Yes | Four-channel means; returned as `meanChn0` ~ `meanChn3` by `aclmdlGetFirstAippInfo` |
| `min_chn_0` ~ `min_chn_3` | number | Yes | Four-channel minimum values; returned as `minChn0` ~ `minChn3` by `aclmdlGetFirstAippInfo` |
| `var_reci_chn_0` ~ `var_reci_chn_3` | number | Yes | Four-channel reciprocal variances; returned as `varReciChn0` ~ `varReciChn3` by `aclmdlGetFirstAippInfo` |
| `support_rotation` | integer | Yes | Rotation support switch; parsed and stored in `AippConfigInfo`, available through internal `GetAippInfo`. Currently not mapped to ACL query results by `SetAippInfo` or read by dynamic AIPP validation |
| `related_input_rank` | integer | Yes | Associated input rank; `GetAndCheckAippParamsOm2` in `aclmdlSetInputAIPP` uses it to locate the original input and validate AIPP output batch, width, and height. `aclmdlSetAIPPByInputIndex` indirectly reuses this validation |
| `max_src_image_size` | integer | Yes | Maximum source image size; `GetAndCheckAippParamsOm2` in `aclmdlSetInputAIPP` uses it to validate the byte limit for user dynamic AIPP source images. `aclmdlSetAIPPByInputIndex` indirectly reuses this validation |
| `aipp_inputs` / `aipp_outputs` | `array<string>` | Yes | Input/output descriptions before/after AIPP; each item has the format `format:data_type:name:size:dim_num:dim0,dim1,...`. `aclmdlGetFirstAippInfo` returns dimensions and byte sizes for each gear through `GetAllAippInputOutputDims`. `aclmdlSetInputAIPP` also uses input dimensions to validate the output shape; `ParseAippDimInfo` does not read the format/data_type parts of the string |
| `orig_input_format` | integer | Yes | Original input format; returned as `srcFormat` by `aclmdlGetFirstAippInfo` through `GetOrigInputInfo`, and used during `aclmdlSetInputAIPP` validation to identify N, H, and W positions in NCHW/NHWC |
| `orig_input_data_type` | integer | Yes | Original input data type; returned as `srcDatatype` by `aclmdlGetFirstAippInfo` through `GetOrigInputInfo` |
| `orig_input_dim_num` | integer | Yes | Original input dimension count; returned as `srcDimNum` by `aclmdlGetFirstAippInfo` through `GetOrigInputInfo` |

### 2.3 op_attr.json

Currently, `Om2PackageHelper::BuildDebugInfo` collects only nonempty original operator name lists, organized by operator name. The attribute key is defined by `ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES`; do not assume that all operator attributes are included. The content is `{}` when no matching attributes exist.

| JSON Field | Type | Required | Description |
|-----------|------|------|------|
| `<op_name>` | object | No (written when a nonempty original operator name list is matched) | Attribute set for an operator name; `ParseOpAttrJsonToMapInternal` organizes attributes by operator name. After caching by `aclmdlGetDesc`, `aclmdlGetOpAttr` queries them using `opName` |
| `<op_name>.<attr_name>` | object | Yes | Attribute key is the string corresponding to `ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES`; `aclmdlGetOpAttr` matches this key using its `attr` argument |
| `<op_name>.<attr_name>.type` | string | Yes | Currently fixed to `LIST_STRING`; `ParseOpAttrJsonToMapInternal` selects the string-list encoding accordingly as the return format of `aclmdlGetOpAttr` |
| `<op_name>.<attr_name>.value` | `array<string>` | Yes | Original operator names before compilation, used for dump and related associations; `ParseOpAttrJsonToMapInternal` encodes each item as `[string length]string` and concatenates the items for `aclmdlGetOpAttr`, rather than returning the JSON array directly |

### 2.4 constants_config.json

| JSON Field | Type | Required | Description |
|-----------|------|------|------|
| `internal_weight_size` | integer | Yes | Total internal weight size in bytes with uint64 semantics, or 0 if there are no internal weights; serves the weight size returned by `aclmdlQuerySize` / `aclmdlQuerySizeFromMem` and the submodel weight size returned by `aclmdlBundleGetSize`. `PrepareInternalConsts` uses it to allocate or validate internal weight memory at load time |
| `consts` | object | Yes | Constant configuration map, possibly empty; not an array. `DeserializeConstantsConfig` and `PrepareConstantsFromStruct` prepare the constant address table for ACL model loading and pass it to `GertModelLoad` |
| `consts.<key>` | object | No (written when the corresponding constant exists) | Keys for `INTERNAL` constants are `constant_<index>`; other sources use operator names. `DeserializeConstantsConfig` iterates object values to restore constant entries; the current runtime address table is indexed by the entry's `index`, not the object key |
| `consts.<key>.index` | integer | Yes | Constant index with uint64 semantics; `PrepareConstantsFromStruct` and the various `Prepare*Consts` interfaces use it to determine the length and slots of the `GertModelLoadConfig.constants` address table |
| `consts.<key>.type` | string | Yes | Constant source type; `ClassifyConstItems` dispatches `INTERNAL`, `COMBINED`, and `INDIVIDUAL` to preparation interfaces for internal weights, combined external weights, and individual external weights, respectively. Unknown types cause loading to fail |
| `consts.<key>.file_name` | string | Yes | Data source file name; `FindInternalWeightBuf` matches constant files in the package, while `PrepareCombinedConsts` / `PrepareIndividualConsts` match external weight files or user-provided file-constant memory. Associated with the `aclmdlSetExternalWeightAddress` configuration |
| `consts.<key>.file_path` | string | No (written for non-offline serialization when the type is not INTERNAL and the path is nonempty) | `RewriteOm2ConstantsConfig` in `RepackOm2ModelData` uses it to relocate external weights when saving a model and removes this field. The executor's `ParseConstItems` does not read this member; it is not the path source for ACL offline loading |
| `consts.<key>.offset` | integer | Yes | Byte offset in the data source with int64 semantics; the various `Prepare*Consts` interfaces use it to locate internal weight addresses, external weight slices, or data in user file-constant memory |
| `consts.<key>.size` | integer | Yes | Constant byte size with int64 semantics; the various `Prepare*Consts` interfaces use it for weight boundary and user memory size validation, and for external weight reading, allocation, and copying |

### 2.5 variables_config.json

Generated when the configuration object exists and `var_metas` or `entries` is nonempty. Variable initialization data is stored in `data/variables/var_weight_data_N`, not directly in JSON.

Except for explicitly noted query uses, the runtime variable interfaces below are called by `Om2ModelExecutor::Impl::PrepareVariablesFromStruct` / `PrepareVarAddrs` during ACL single-model or Bundle submodel loading, ultimately passing variable addresses to `GertModelLoadConfig.var_addrs`; they are not public per-field query interfaces.

| JSON Field | Type | Required | Description |
|-----------|------|------|------|
| `graph_id` | integer | Yes | Graph ID with uint64 semantics; `PrepareVariablesFromStruct` passes it to `Om2RTVarManager::TransAllVarData` to select variables requiring transformation for this graph |
| `global_shared_var_size` | integer | No (written when greater than 0) | Global shared variable HBM size in bytes; `GetOm2BundleInfo` takes the maximum across submodels for `aclmdlBundleGetVarWeightSize`, and for `aclmdlBundleInitFromFile` / `aclmdlBundleInitFromMem` to validate external variable memory size |
| `var_metas` | `array<object>` | Yes | Variable metadata, possibly empty; `ValidateVarMetas` validates indices, `PrepareVarAddrs` constructs the SO variable address table, and `PrepareVariablesFromStruct` collects names of variables involved in transformations and copies |
| `var_metas[].index` | integer | Yes | Variable index; `ValidateVarMetas` checks bounds and duplicates, and `PrepareVarAddrs` writes the device address to the corresponding address table slot |
| `var_metas[].var_name` | string | Yes | Variable name; `PrepareVarAddrs` looks up the device address through `Om2RTVarManager::GetVarDevAddr`, and `TransAllVarData` / `CopyVarData` also use it to locate variables |
| `var_metas[].op_type` | string | Yes | Operator type; parsed and stored by `ParseVarMetaFromJson`. Currently, `PrepareVarAddrs` and variable initialization do not branch on this field |
| `var_metas[].op_name` | string | Yes | Operator name; parsed and stored by `ParseVarMetaFromJson`. Currently unused by variable address preparation, transformation, and copying, with no corresponding ACL query interface |
| `var_metas[].tensor_desc` | object | Yes | Tensor description; fields are listed at the end of this table. Parsed and stored by `ParseVarMetaFromJson`; currently `PrepareVarAddrs` obtains addresses by variable name without using this description |
| `entries` | object | No (written when entries is nonempty) | Variable resource map; `DeserializeVariablesData` restores entries and initialization data, and `Om2RTVarManager::Init` registers and merges shared variable resources |
| `entries.<var_key>` | object | Yes | Resource entry indexed by variable key; `DeserializeVariablesData` iterates object values without using the outer key. `RTVarAddEntry` and `Om2RTVarManager::Init` use the entry's `var_key` |
| `entries.<var_key>.var_name` | string | Yes | Variable name; `RTVarResource::AddEntry` builds the name-to-variable-key mapping for `GetEntryByName`, `GetVarDevAddr`, variable transformation, and copying |
| `entries.<var_key>.file_name` | string | Yes | Initialization data file basename, such as `var_weight_data_0`; `DeserializeVariablesData` uses this value from the first entry to locate the shared initialization data file under `data/variables/`, then slices each entry from that file |
| `entries.<var_key>.var_key` | string | Yes | Variable key corresponding to the object key; `RTVarAddEntry` checks that it is nonempty, `RTVarResource::AddEntry` uses it as the resource index, and `Om2RTVarManager::Init` uses it for Bundle shared-variable deduplication and consistency validation |
| `entries.<var_key>.op_type` | string | Yes | Operator type; parsed and stored by `ParseVarEntryFromJson` and retained by `CopyVarEntry`. Currently, `Om2RTVarManager` does not use it to determine allocation, transformation, or copy logic |
| `entries.<var_key>.logic_addr` | integer | Yes | Logical address with uint64 semantics; `Om2RTVarManager::GetVarDevAddr` calculates the offset relative to the variable base address when using external variable memory. `Init` also uses it for shared-variable consistency validation |
| `entries.<var_key>.size` | integer | Yes | Variable byte size with uint64 semantics; `AllocDevAddr` uses it for device memory allocation, `GetVarDevAddr` for external memory boundary and initialization size validation, and `CopyVarFromDevice` / `CopyVarToDevice` for variable data copies |
| `entries.<var_key>.memory_type` | integer | Yes | Memory type; `Om2RTVarManager::AllocDevAddr` converts it to `rtMemType_t` and passes it to `Om2Malloc`. `Init` also uses it for shared-variable consistency validation |
| `entries.<var_key>.changed_graph_id` | integer | Yes | Graph ID associated with a variable change; `TransAllVarData` checks further transformation conditions only when this equals the current `graph_id` and differs from `allocated_graph_id` |
| `entries.<var_key>.allocated_graph_id` | integer | Yes | Graph ID associated with variable allocation; `TransAllVarData` compares it with `changed_graph_id` to determine whether cross-graph variable transformation is needed |
| `entries.<var_key>.tensor_desc` | object | Yes | Variable Tensor description; `Om2RTVarManager::Init` validates shared-variable consistency using shape, type, and format. `CopyVarData` uses source/target types to determine whether conversion is needed and the source shape to calculate element count |
| `entries.<var_key>.trans_road` | `array<object>` | Yes | Variable transformation node sequence, possibly empty; after filtering by `TransAllVarData`, `TransSingleVarData` and `TransVarOnHost` perform host-side format or type conversion |
| `entries.<var_key>.trans_road[].node_type` | string | Yes | Transformation node type; `NeedRealTrans` / `TransVarOnHost` use it to skip nodes that need no conversion or select conversion logic such as `TransDataFormat` or `TransTensorDataType` |
| `entries.<var_key>.trans_road[].input` / `output` | object | Yes | Tensor descriptions before/after transformation; `TransSingleVarData` constructs the old variable key from the first node's input description, while `TransVarOnHost` constructs transformation parameters from shape, format, and type |
| `entries.<var_key>.copy_info` | object | Yes | Variable copy information; used by `Om2RTVarManager::CopyVarData` to restore data copy relationships between variables |
| `entries.<var_key>.copy_info.src_var_name` | string | Yes | Source variable name; `CopyVarData` passes it with the source description to `RTVarResource::BuildVarKey` to find the source variable |
| `entries.<var_key>.copy_info.src_tensor_desc` | object | Yes | Source Tensor description; `CopyVarData` constructs the source variable key from its format and type. The actual copy size and type conversion are taken from the source resource entry found |
| `entries.<var_key>.init_data_offset` | integer | Yes | Byte offset of initialization data in the weight file, or 0 when there is no data; after range validation, `ParseVarEntryFromJson` restores `entry.init_data` at this offset for `GetVarDevAddr` to copy to the device during first initialization |
| `entries.<var_key>.init_data_size` | integer | Yes | Initialization data size in bytes, or 0 when there is no data; used by `ParseVarEntryFromJson` for slicing and boundary validation. `GetVarDevAddr` initializes the variable with the restored data through `aclrtMemcpy` |
| Tensor description `name` | string | Yes | Tensor name; parsed and stored by `ParseTensorDescFromJson`. Currently unused by variable key construction, format conversion, and variable copying |
| Tensor description `shape` | `array<integer>` | Yes | Dimension sizes; used in resource entries for `Init` consistency validation and `CopyVarData` element count calculation, and in transformation node descriptions for `TransVarOnHost`. Only stored in `var_metas` |
| Tensor description `data_type` / `format` | integer | Yes | Same enum values as model_meta Tensor descriptions; used by `RTVarBuildKey` to construct variable keys, by `Init` for consistency validation, and by `TransVarOnHost` / `CopyVarData` to determine transformation parameters. Only stored in `var_metas` |
| Tensor description `size` | integer | Yes | Tensor byte size; parsed and stored by `ParseTensorDescFromJson`. Current runtime variable allocation and copying mainly use the resource entry's `entries.<var_key>.size`, not this description member |
| Tensor description `shape_range` | `array<array<integer>>` | Yes | `[min,max]` for each dimension; parsed and stored by `ParseTensorDescFromJson`. Currently unused by variable address preparation, format conversion, and variable copying; not the data source for `aclmdlGetInputDimsRange` |

### 2.6 debug/ge_visual_00000000_graph_0.json

Generated by `VisualJsonConverter::SerializeFromGeModel`, with an outer wrapper and a GE model visualization object. The contents of `model` are recursively converted according to model Proto descriptors, and the fields present depend on model contents; this is not the simplified Tensor format of model_meta.

This file is extracted as a whole through `DeserializeGertVisualJson` and `Om2PackageHelper::ExtractVisualJson`; `DeserializeGertModelData` used for normal model loading does not read it. The model and attribute fields below are recursively converted back to Proto JSON by `VisualJsonConverter::LoadFromVisualJson` for model visualization and debugging tools, not as data sources for `aclmdlGetDesc`, model execution, or AIPP queries.

| JSON Field | Type | Required | Description |
|-----------|------|------|------|
| `format` | string | Yes | Fixed to `ge_visual_json` for tools to identify the format; currently, `LoadFromVisualJson` does not validate or select conversion logic based on this field |
| `format_version` | integer | Yes | Currently 1, recording the visualization format version; currently, `LoadFromVisualJson` does not branch on it or perform version validation |
| `model` | object | Yes | Visualization representation of a GE model, with fields derived from `ModelDef`; `LoadFromVisualJson` checks that it exists and starts recursive conversion using the `ModelDef` descriptor |
| `model.name` | string | No (written when provided by the source model) | Model name; `LoadFromVisualJson` restores it through `FromVisualRecursive` for debugging display, with no corresponding ACL name query |
| `model.graph` | `array<object>` | No (written when the source model contains graphs) | Graph collection; `FromVisualRecursive` recursively restores graphs according to Proto graph field descriptors for display by visualization tools |
| `model.graph[].name` | string | No (written when provided by the source graph) | Graph name; restored by `FromVisualRecursive` for debugging display |
| `model.graph[].op` | `array<object>` | No (written when the source graph contains operators) | Operators in the graph; recursively restored by `FromVisualRecursive` according to operator descriptors for graph structure display |
| `model.graph[].op[].name` / `type` | string | No (written when provided by the source operator) | Operator name and type; restored by `FromVisualRecursive` as operator identifiers for graph node display, not for `aclmdlGetOpAttr` queries |
| `model.graph[].op[].input` | `array<string>` | No (written when the source operator contains input connections) | Operator input connection information; restored by `FromVisualRecursive` for visualization tools to associate nodes |
| `model.graph[].op[].input_desc` / `output_desc` | `array<object>` | No (written when the source operator contains corresponding Tensor descriptions) | Recursively converted model Proto Tensor description objects; restored by `FromVisualRecursive` for node information display, not for ACL model input/output queries |
| `attr` at each level | object | No (written when corresponding attributes exist) | Map from attribute names to visualization attribute values; `ProcessObjectFromVisual` / `FromVisualRecursive` recursively restore Proto attributes according to attribute descriptors |
| Scalar attribute values | string / integer / number / boolean | No (written when scalar attributes exist) | Scalar attributes use the corresponding JSON values; `WrapAttrDef` restores the corresponding Proto attribute wrappers |
| Compound attribute `type` | string | Yes | Non-scalar attribute type identifier, such as `bytes`, `expression`, `tensor_desc`, `tensor`, `graph`, or `func`; lists directly use the `list_*` identifiers below. `WrapAttrDef` identifies the attribute type from this field |
| Compound attribute `value` | string / object / array | Yes | Content corresponding to the type, with nested objects recursively converted; `WrapAttrDef` restores attribute contents using `type`, and `FromVisualRecursive` recursively processes nested objects |
| List value `type` | string | Yes | Such as `list_string`, `list_int`, `list_float`, or `list_tensor_desc`; `WrapAttrDef` / `WrapListValue` select the Proto field for list elements accordingly |
| List value `value` | array | Yes | Element array of the corresponding type; `WrapListValueByType` restores the list wrapper, and `ConvertListValueElementsFromVisual` recursively restores message-type elements |
| Two-dimensional integer/floating-point array attributes | `array<array<number>>` | No (written when corresponding two-dimensional array attributes exist) | Emitted directly as nested arrays without a `type`/`value` wrapper; `WrapAttrDef` identifies integer or floating-point two-dimensional lists from the first element's type, then restores the Proto wrapper through `WrapNestedList` |
