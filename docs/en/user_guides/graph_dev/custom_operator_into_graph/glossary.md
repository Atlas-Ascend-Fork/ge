# Glossary

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:38:00.138Z pushedAt=2026-10-28T03:30:34.502Z nl=2 -->

| Glossary | Description |
| --- | --- |
| GE | Graph Engine, responsible for graph compilation and execution. |
| Custom operator | A mechanism for developers' own kernels to be integrated into the GE graph compilation/execution process, loaded as a .so deliverable. |
| op type | The type identifier of an operator in a GE graph. It must be consistent across REG_OP/REG_AUTO_MAPPING_OP/the graph construction side. |
| REG_OP | A GE operator prototype registration macro that declares the operator's inputs, outputs, and attributes. |
| REG_AUTO_MAPPING_OP | A registration macro that maps a C++ implementation class to an op type. |
| IMPL_OP | A registration macro that attaches an implementation (such as .InferShape(...)) to a registered operator. |
| BaseCustomOp | The common base class of custom operator implementation classes. |
| EagerExecuteOp | A capability interface that provides the Execute (runtime execution) callback. |
| ShapeInferOp | A capability interface that provides the InferShape/InferDataType (compile-time inference) callbacks. |
| CompilableOp | A capability interface that provides the Compile (online compilation) callback. |
| PortableOp | A capability interface that provides the Serialize/Deserialize (OM serialization/deserialization) callbacks. |
| ArgsUpdater | A capability interface that provides the UpdateHostArgs (I/O address refresh) callback. |
| OPP | Operator Package, the operator package directory specification, loaded via ASCEND_CUSTOM_OPP_PATH. |
| ASCEND_CUSTOM_OPP_PATH | An environment variable that specifies the root directory of the custom operator OPP package. |
| ASCEND_HOME_PATH | An environment variable for the CANN installation root directory, used by CMake to locate headers/libraries. |
| AIR | Ascend Intermediate Representation, the intermediate representation format for GE graph compilation. |
| OM | Offline Model, the offline model file produced by ATC compilation. |
| ATC | Ascend Tensor Compiler, the offline model compilation tool. |
| acl | The Ascend Computing Language runtime library (aclrt* APIs). |
| RTC | Runtime Compilation, compiling kernel source code into a device binary at runtime. |
| Workspace | Device-side temporary memory, allocated via ctx->MallocWorkSpace. |
| shape buffer | The device memory where the kernel writes back the actual shape in scenario 4, in the format [ndim, d0, d1, ...]. |
| FX Graph | PyTorch's graph representation (torch.fx), the graph input of TorchAir. |
| TorchAir | A TorchNPU third-party library responsible for FX Graph → GE Graph conversion (the converter mechanism). |
| converter | A registration function that maps PyTorch FX nodes to GE operator types. |
| TORCH_LIBRARY | A registration macro/interface for PyTorch operator schemas and implementations. |
| TORCH_LIBRARY_IMPL | Binds implementations by DispatchKey (PrivateUse1/Meta/XLA, etc.). |
| PrivateUse1 | The PyTorch custom device (NPU) dispatch key. |
| Adapter | The GE graph adaptation layer for the TensorFlow scenario, which can automatically generate GE operator prototypes from TF op_def. |
| npu_supported_ops.json | The operator visibility configuration file of the Adapter in the TF scenario. |
| Parser | The parser for ONNX model → GE graph. |
| NodeProto | The node definition in an ONNX model. |
| ParseParamsFn | The callback function responsible for attribute mapping in the ONNX parsing plugin. |
| RTC compile-time callback | The mechanism by which the Compile callback is invoked during GE compilation (scenarios 2/3). |
| stream | The device execution stream on which the kernel executes asynchronously. |
| GetShapeSize()/GetSize() | The former returns the number of elements, and the latter returns the number of bytes; use the former at launch. |
