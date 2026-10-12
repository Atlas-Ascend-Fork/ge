# 术语表

| 术语 | 说明 |
| --- | --- |
| GE | Graph Engine，图引擎，负责图编译与执行。 |
| 自定义算子 | 开发者自有kernel接入GE图编译/执行流程的机制，以.so交付件加载。 |
| op type | 算子在GE图中的类型标识，REG_OP/REG_AUTO_MAPPING_OP/构图侧三处必须一致。 |
| REG_OP | GE算子原型注册宏，声明算子的输入、输出、属性。 |
| REG_AUTO_MAPPING_OP | 将C++实现类映射到op type的注册宏。 |
| IMPL_OP | 为已注册算子挂接实现（如 .InferShape(...)）的注册宏。 |
| BaseCustomOp | 自定义算子实现类的公共基类。 |
| EagerExecuteOp | 提供Execute（运行时执行）回调的能力接口。 |
| ShapeInferOp | 提供InferShape/InferDataType（编译期推导）回调的能力接口。 |
| CompilableOp | 提供Compile（在线编译）回调的能力接口。 |
| PortableOp | 提供Serialize/Deserialize（OM序列化/反序列化）回调的能力接口。 |
| ArgsUpdater | 提供UpdateHostArgs（I/O地址刷新）回调的能力接口。 |
| OPP | Operator Package，算子包目录规范，经ASCEND_CUSTOM_OPP_PATH加载。 |
| ASCEND_CUSTOM_OPP_PATH | 指定自定义算子OPP包根目录的环境变量。 |
| ASCEND_HOME_PATH | CANN安装根目录环境变量，CMake定位头文件/库。 |
| AIR | Ascend Intermediate Representation，GE图编译的中间表示格式。 |
| OM | Offline Model，ATC编译产出的离线模型文件。 |
| ATC | Ascend Tensor Compiler，离线模型编译工具。 |
| acl | Ascend Computing Language运行时库（aclrt*系列API）。 |
| RTC | Runtime Compilation，运行期将kernel源码编译为device binary。 |
| Workspace | 设备侧临时内存，经ctx->MallocWorkSpace分配。 |
| shape buffer | 场景4中kernel 回写实际shape的设备内存，格式[ndim, d0, d1, ...]。 |
| FX Graph | PyTorch的图表示（torch.fx），TorchAir的入图输入。 |
| TorchAir | TorchNPU三方库，负责FX Graph → GE Graph转换（converter 机制）。 |
| converter | 将PyTorch FX节点映射到GE算子类型的注册函数。 |
| TORCH_LIBRARY | PyTorch算子schema与实现的注册宏/接口。 |
| TORCH_LIBRARY_IMPL | 按DispatchKey绑定实现（PrivateUse1/Meta/XLA等）。 |
| PrivateUse1 | PyTorch自定义设备（NPU）dispatch key。 |
| Adapter | TensorFlow场景的GE图适配层，可从TF op_def自动生成GE算子原型。 |
| npu_supported_ops.json | TF场景Adapter的算子可见性配置文件。 |
| Parser | ONNX模型 → GE图的解析器。 |
| NodeProto | ONNX模型中的节点定义。 |
| ParseParamsFn | ONNX解析插件中负责属性映射的回调函数。 |
| RTC编译期回调 | Compile回调在GE编译期被调用的机制（场景2/3）。 |
| stream | 设备执行流，kernel在其上异步执行。 |
| GetShapeSize()/GetSize() | 前者返回元素个数，后者返回字节数；launch时用前者。 |
