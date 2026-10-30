# Header and Library Files

<!-- md-trans-meta sourceCommit=9d821ee607484f54716ece49d9dbdbe1c4f9524d translatedAt=2026-10-15T04:21:16.771Z pushedAt=2026-10-16T10:53:07.736Z -->

The header files of the GE graph engine APIs are located in the following directories:

- Compilation APIs:
  - `${INSTALL_DIR}/include/ge/`
  - `${INSTALL_DIR}/include/parser/`
  - `${INSTALL_DIR}/include/acl/`

- Execution APIs:
  - `${INSTALL_DIR}/include/ge/`
  - `${INSTALL_DIR}/include/graph/`
  - `${INSTALL_DIR}/include/external/`
  - `${INSTALL_DIR}/include/acl/`
  - `${INSTALL_DIR}/include/register/`
  - `${INSTALL_DIR}/include/transformer/`
  - `${INSTALL_DIR}/pkg_inc/register`

The library files are located in the `${INSTALL_DIR}/lib64/` directory. Replace `${INSTALL_DIR}` with the file storage path after the CANN software is installed. For example, when installed as the root user, the default storage path is **/usr/local/Ascend/cann**.

## ge-compiler Compilation APIs

| Header File | Description | Library File |
| --- | --- | --- |
| ge/hcom_gradient_split_tune.h | Defines the HCOM gradient split tuning metrics, input, and result APIs. | libhcom_gradient_split_tune.so |
| ge/hcom_ops_stores.h | Defines the HCOM plugin initialization and OpsKernelInfoStore/GraphOptimizer query APIs. | libhcom_graph_adaptor.so |
| ge/ge_ir_build.h | Defines the aclgrphBuild*, model compilation, saving, and weight-refreshable graph APIs. | libge_compiler.so |
| ge/ge_utils.h | Defines the GE graph modification, Shape inference, node support validation, and other utility APIs. | libge_compiler.so |
| ge/fusion/graph_rewriter.h | Defines the subgraph replacement and graph modification APIs. | libge_compiler.so |
| ge/fusion/match_result.h | Defines the pattern match results and captured node/tensor access. | libge_compiler.so |
| ge/fusion/graph_fuse_inspector_utils.h | Defines the pre-fusion validity check and fusion result reporting. | libge_compiler.so |
| ge/fusion/pattern.h | Defines the graph pattern to be matched and captured tensors. | libge_compiler.so |
| ge/fusion/pattern_matcher.h | Defines the pattern matching on the target graph. | libge_compiler.so |
| ge/fusion/subgraph_boundary.h | Describes subgraph inputs, outputs, and boundaries. | libge_compiler.so |
| ge/fusion/pattern_matcher_config.h | Configures PatternMatcher matching behavior. | libge_compiler.so |
| ge/fusion/infer_shape_util.h | Defines the shape inference helper APIs for fused/replaced graphs. | libge_compiler.so |
| ge/fusion/pass/decompose_pass.h | Defines the one-to-many operator decomposition Pass APIs and registration macros. | libge_compiler.so |
| ge/fusion/pass/fusion_base_pass.h | Defines the pure abstract base class for custom fusion Passes. | Header-only APIs; the concrete Pass framework consumes libge_compiler.so |
| ge/fusion/pass/fusion_pass_reg.h | Defines the fusion Pass registration data and registrar. | libge_compiler.so |
| ge/fusion/pass/pattern_fusion_pass.h | Defines the pattern fusion Pass base class and replacement flow. | libge_compiler.so |
| parser/caffe_parser.h | Parses Caffe models into GE Graphs. | libfmk_parser.so |
| parser/onnx_parser.h | Parses ONNX models into GE Graphs. | libfmk_onnx_parser.so |
| parser/tensorflow_parser.h | Parses TensorFlow models into GE Graphs. | libfmk_parser.so |
| ge/ge_api.h | Defines the GE initialization, Session, graph add/compile/execute, and other V1 APIs. | libge_runner.so |
| ge/ge_api_v2.h | Defines the GESession, V2 graph loading and execution APIs. | libge_runner_v2.so |
| ge/ge_feature_memory.h | Defines the feature memory description of compilation results. | libge_common_base.so (actual symbol ownership; the upper-level compilation flow also uses libge_compiler.so) |
| ge/ge_graph_compile_summary.h | Defines the compilation result, stream allocation, and external weight summary. | libge_compiler.so |
| ge/esb_funcs.h | Defines the ES graph builder C ABI. | libeager_style_graph_builder_base.so |
| ge/compliant_node_builder.h | Defines the chained Builder for constructing nodes according to the IR specification. | libeager_style_graph_builder_base.so |
| ge/es_c_graph_builder.h | Defines the ES C Graph Builder wrapper. | libeager_style_graph_builder_base.so |
| ge/es_c_tensor_holder.h | Defines the ES C TensorHolder wrapper. | libeager_style_graph_builder_base.so |
| ge/es_graph_builder.h | Defines the C++ templated ES Graph Builder facade. | Header-only facade; calls the C ABI of libeager_style_graph_builder_base.so |
| ge/es_tensor_holder.h | Defines the C++ ES TensorHolder APIs. | libeager_style_graph_builder_base.so |
| ge/es_tensor_like.h | Defines the ES TensorLike description and conversion APIs. | libeager_style_graph_builder_base.so |
| acl/acl_op_compiler.h | Defines the single-operator compilation, compile-and-execute, and compilation option APIs. | libacl_op_compiler.so |

## ge-executor Execution APIs

| Header File | Description | Library File |
| --- | --- | --- |
| ge/ge_api_types.h | Defines the aggregation entry for GE common status, tensor, graph, and Session-related types. | Header-only aggregation header; link libgraph.so/libgraph_base.so and others based on the actual API. |
| ge/ge_api_error_codes.h | Defines the GEAPI error code registration and string conversion APIs. | libge_common_base.so |
| ge/ge_external_weight_desc.h | Defines the APIs for describing the location, length, and attributes of external weights. | libge_common_base.so |
| external/ge_common/ge_api_types.h | Defines the GE API type aggregation entry compatible with legacy paths. | Header-only compatibility header; link the corresponding library based on the actual API. |
| external/ge_common/ge_common_api_types.h | Defines the GE common enums, structs, and base API types. | Header-only definition header. |
| acl/acl_mdl.h | Defines the ACL model loading, execution, dynamic Shape, and model configuration APIs. | libacl_mdl.so (public facade; at runtime it also depends on the model execution implementation library). |
| acl/acl_base_mdl.h | Defines the ACL model base types, datasets, and description APIs. | libacl_mdl.so (public facade). |
| acl/acl_op.h | Defines the ACL single-operator description, execution, and operator attribute APIs. | libacl_op_executor.so (public facade; internal implementation dependencies are not direct link APIs for applications). |
| acl/ops/acl_cblas.h | Defines the ACL CBLAS/matrix computation APIs. | libacl_cblas.so |
| exe_graph/runtime/eager_op_execution_context.h | Defines the input, output, Stream, and Workspace context for Eager operator execution. | liblowering.so |
| exe_graph/runtime/host_cpu_op_execution_context.h | Defines the input, output, and Host memory management context for Host CPU operator execution. | liblowering.so |
| exe_graph/runtime/op_compile_context.h | Defines the operator compilation, Tiling, and Shape inference context. | liblowering.so |
| graph/graph.h | Defines the GE Graph creation, node addition/removal, input/output, and graph attribute APIs. | libgraph.so |
| graph/ct_infer_shape_range_context.h | Defines the compile-time Shape Range inference context. | Header-only API; primary consumer library libgraph.so. |
| graph/ct_infer_shape_context.h | Defines the compile-time Shape inference context. | Header-only API; primary consumer library libgraph.so. |
| graph/operator_reg.h | Defines the registration macros for operator prototypes, inputs/outputs, attributes, and inference functions; provides GetRegisteredIrDefV2 to query registered IR operator prototype definitions. | libgraph.so is the primary registration implementation, using libgraph_base.so base capabilities. |
| graph/gnode.h | Defines the Graph node query, connection relationship, and attribute access APIs. | libgraph.so |
| graph/graph_buffer.h | Defines the APIs for creating, reading/writing, and managing the lifecycle of graph serialization buffers. | libgraph_base.so |
| graph/inference_context.h | Defines the context, Marks, and resource management APIs for Shape/Value inference. | libgraph.so |
| graph/attr_value.h | Defines the Graph attribute value types and serialization access APIs. | libgraph.so |
| graph/operator.h | Defines the GE Operator creation, connection, attribute, and description access APIs. | libgraph_base.so |
| graph/operator_factory.h | Creates Operators by type and queries operator registration information. | libgraph.so |
| graph/resource_context.h | Defines the abstract APIs and resource keys for the graph resource context. | Defines the Header-only API and primary consumer library libgraph.so. |
| graph/kernel_launch_info.h | Defines the Kernel Launch parameters, handles, and registration information. | libregister.so |
| graph/arg_desc_info.h | Defines the kernel parameter description information. | libgraph.so |
| graph/custom_op.h | Defines the registration, inference, and validation macros for custom operators. | libgraph.so (primary public registration symbols). |
| graph/hcom_executor.h | Defines the HCOM graph executor initialization, Finalize, and execution APIs. | libhcom_executor.so |
| register/register_base.h | Defines the base classes, macros, and priority definitions for the GE registration mechanism. | libregister.so |
| register/op_lib_register.h | Defines the operator library, OpInfo, and OpsKernelInfoStore registration APIs. | libregister.so |
| register/register_custom_pass.h | Defines the custom graph Pass registration and execution timing configuration. | libregister.so |
| register/op_binary_resource_manager.h | Defines the operator binary resource registration, query, and lifecycle management. | libregister.so |
| register/scope/scope_fusion_pass_register.h | Defines the Scope Fusion Pass registration, rules, and result APIs. | libregister.so |
| transformer/transfer_def.h | Defines the common structs and enums for Format/Shape conversion. | Header-only definition header; primary consumer library libgraph.so. |
| transformer/transfer_shape_according_to_format_ext.h | Converts Shape according to the source/target Format. | libgraph.so |
| pkg_inc/register/amct_interface.h | AMCT plugin callbacks and quantization-related abstract APIs. | Defines the Header-only plugin API; primary consumer library on the host side is libregister.so. |
| pkg_inc/register/amct_registry.h | Defines the AMCT plugin registration, query, and instance creation. | libregister.so |
