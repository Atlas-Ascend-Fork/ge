# 调试与常见问题

## 开发检查项（按入图阶段）

- **阶段1-2（定义与桥接）：**
  - 算子类型名、注册类名、构图侧op type三者一致。
  - PyTorch场景：TORCH\_LIBRARY schema与REG\_OP输入输出一致。
  - converter中的GE op type与REG\_AUTO\_MAPPING\_OP注册名一致。

- **阶段3（入图）：**
  - PyTorch：converter已注册，torchair.ge\_mode\(\)生效。
  - ONNX：解析插件已加载（ATC 编译前），OriginOpType 格式为 domain::version::OpType。

- **阶段4-5（编译与执行）：**
  - ASCEND\_CUSTOM\_OPP\_PATH指向OPP包根目录。
  - op\_graph/lib/<os\>/<arch\>/下有.so。
  - kernel binary/源码路径不依赖临时工作目录。
  - 离线OM场景确认soc\_version与实际环境一致。

## 常见错误排查

| 现象 | 可能原因 | 排查方法 |
| --- | --- | --- |
| GE找不到自定义算子 | ASCEND_CUSTOM_OPP_PATH未配置或路径错误 | 检查环境变量，确认op_graph/lib/\<os>/\<arch>/下有.so。 |
| Execute返回null input | 输入索引与REG_OP定义不匹配 | 核对REG_OP的INPUT顺序和Execute中的GetInputTensor索引。 |
| 编译期shape为空 | Compile阶段输入shape未就绪 | 检查是否在Compile中正确读取GetInputTensor。 |
| OM加载后Execute失败 | Deserialize未正确恢复binary | 检查Serialize/Deserialize的buffer格式是否对称。 |
| PyTorch图模式报错 | converter未注册或op type名不匹配 | 确认register_fx_node_ge_converter中的GE op type与REG_AUTO_MAPPING_OP一致。 |
| 结果数值错误（可跑通但输出不对） | launch参数单位错误（字节数/元素数混用） | 元素数用GetShapeSize()，不要用GetSize()；核对block配置。 |
| 结果数值错误（广播相关） | InferShape广播实现不合法 | 按标准广播规则（尾部对齐、维度为1才可扩展），非法时返回GRAPH_FAILED。 |
| ONNX转ATC报PARAM_INVALID | Parser不认识该op type | 确认解析插件已加载、OriginOpType的domain/version匹配模型。 |
| 构建报“符号找不到/签名不匹配” | CANN/GE版本与文档示例不一致 | 核对版本配套，以目标版本头文件为准。 |

## 日志与错误码

- **CANN运行日志（plog）**：通过环境变量控制日志级别，排查GE加载.so、实例化算子、回调调用失败等底层问题。

    ```bash
    export ASCEND_GLOBAL_LOG_LEVEL=1   # 0=DEBUG 1=INFO 2=WARNING 3=ERROR
    export ASCEND_SLOG_PRINT_TO_STDOUT=1
    ```

- **错误码范围**：运行时错误码形如161xxx（参数/环境类）、507xxx（aicore 超时类）等。错误码格式为 \[错误码\]\[模块\]\[子模块\]\[错误原因\]，定位时先查“该错误码属于哪个阶段”。
- **编译期日志**：RTC编译失败时，日志中应包含kernel源码编译错误详情。

完整错误码说明请参见《[故障处理](https://gitcode.com/cann/docs/blob/master/docs/zh/troubleshooting/00_troubleshooting.md)》。

## 如何进一步定位

1. **按阶段二分**：确认错误发生在“定义/桥接 → 入图 → 编译 → 执行”哪一段？检查清单可参考[开发检查项（按入图阶段）](#开发检查项按入图阶段)。
2. **核对三处一致**：重点检查REG\_OP、REG\_AUTO\_MAPPING\_OP以及构图侧的op type是否完全匹配，这是最高频的问题根因。
3. **看日志**：开启plog（[日志与错误码](#日志与错误码)）并复现问题，重点关注GE加载过程及回调调用行的日志与错误码。
4. **最小化复现**：从快速开始的Add示例改起，逐步替换为自己的算子，避免一次性引入多个未知因素。
5. **仍无法定位**：请携带环境版本信息和CANN版本、连同完整报错日志一起，向社区或官方支撑渠道反馈。
