# ONNX入图

ONNX前端与PyTorch不同：GE的ONNX Parser不识别的op type会直接报错（PARAM\_INVALID），因此需要额外编写一个**ONNX解析插件**，将ONNX节点的属性映射为GE算子属性。两侧交付件简要示意图如下所示：

![](../figures/onnx_into_graph.png)

1. **阶段1：算子定义**

    算子定义阶段声明算子的输入、输出、属性等元信息，供后续阶段使用。

    **按前端分类的实现方式：**

    | 前端框架 | 定义方式 | 交付件 | 说明 |
    | --- | --- | --- | --- |
    | ONNX | NodeProto | ONNX模型内嵌 | 模型内只声明 op 语义；GE侧REG_OP头文件仍需手写<br> 说明： ONNX 模型内嵌的 NodeProto 只是 op 在模型里的"使用声明"。GE 侧仍需要 op_graph/include/*.h（REG_OP）供 Parser 创建节点时使用。两者职责不同，不能互相替代。 |

2. **阶段2：前端接入（解析插件）**

    将前端算子类型映射到GE算子类型，并处理属性转换。

    **按前端分类的接入方式：**

    | 前端框架 | 接入方式 | 涉及交付件 | 关键点 |
    | --- | --- | --- | --- |
    | ONNX | Parser解析插件 | libonnx_parser_plugin.so | 必须手写ParseParamsFn |

    **ONNX算子类型标识格式**："domain::version::OpType"（如 "ai.onnx::11::Conv"、"com.example::1::MyOp"）。Parser会按此格式在OpRegistry中查找映射，找不到则解析失败。

    **ONNX解析插件需要：**

    1. 使用REGISTER\_CUSTOM\_OP注册ONNX op type到GE op type的映射。
    2. 实现ParseParamsFn，从ONNX NodeProto中提取属性并设置到GE Operator。

    **解析插件代码：**

    ```c++
    // framework/onnx/add_custom_parser.cpp
    #include "register/register.h"
    #include "proto/onnx/ge_onnx.pb.h"

    // 属性解析函数：将ONNX NodeProto属性映射到GE Operator属性
    // op_src：ONNX节点Proto消息体
    // op_dest：GE算子对象，用于设置属性
    static domi::Status ParseMyOp(const google::protobuf::Message *op_src, ge::Operator &op_dest) {
      // 将Message转换为ONNX NodeProto类型
      auto *node = dynamic_cast<const ge::onnx::NodeProto *>(op_src);
      // 遍历ONNX节点的所有属性
      for (const auto &attr : node->attribute()) {
        // 将ONNX中的alpha属性设置到GE算子上
        if (attr.name() == "alpha") {
          op_dest.SetAttr("alpha", attr.f());
        }
      }
      return domi::SUCCESS;
    }

    // 注册ONNX解析插件：将ONNX op type映射到GE op type
    // ONNX Parser遇到不识别的算子时，通过此注册信息查找解析插件
    REGISTER_CUSTOM_OP("MyCustomOp")
        .FrameworkType(domi::ONNX)                                    // 声明为ONNX框架插件
        .OriginOpType({"ai.onnx::11::MyOp", "ai.onnx::13::MyOp"})     // ONNX算子类型（支持多版本）
        .ParseParamsFn(ParseMyOp);                                    // 属性解析回调函数

    ```

    **REGISTER\_CUSTOM\_OP关键方法：**

    | 方法 | 用途 |
    | --- | --- |
    | .FrameworkType(domi::ONNX) | 声明为ONNX框架插件 |
    | .OriginOpType("domain::ver::Type") | 指定ONNX op type（支持列表，覆盖多版本） |
    | .ParseParamsFn(fn) | 属性解析函数（Message* → Operator&） |
    | .ParseParamsByOperatorFn(fn) | 自动映射路径的属性解析（Operator → Operator） |
    | .ParseOpToGraphFn(fn) | 将单个算子展开为子图（可选） |
    | .ParseSubgraphPostFn(fn) | 含子图算子的后处理（可选） |

    ONNX场景需要两个.so：

    | 交付件 | 职责 | 放置路径 |
    | --- | --- | --- |
    | ONNX解析插件.so | REGISTER_CUSTOM_OP+ParseParamsFn，负责ONNX → GE的属性映射 | ASCEND_CUSTOM_OPP_PATH下的插件目录 |
    | GE自定义算子.so | REG_OP+REG_AUTO_MAPPING_OP+执行逻辑 | op_graph/lib/\<os>/\<arch>/ |

    **交付件组织示例：**

    ```tree
    custom_opp/
    ├── op_graph/
    │   ├── include/add_custom.h    # REG_OP
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE交付件
    ├── framework/
    │   └── onnx/
    │       └── libonnx_parser_plugin.so       # 解析插件（独立 .so）
    ```

    交付件统一规范请参见[交付件与OPP规范](deliverables_and_opp.md)。

    **注意事项：**

    - ONNX解析插件.so必须在ATC编译前通过ASCEND\_CUSTOM\_OPP\_PATH加载，否则Parser无法识别自定义op type。
    - OriginOpType中的domain为空时默认使用"ai.onnx"，自定义domain需显式指定。
    - 如果ONNX算子可映射为已有GE算子的组合，可通过ParseOpToGraphFn将其展开为子图，无需编写GE侧自定义算子.so。

3. **阶段3：图构建入图**

    图构建入图阶段将前端计算图转换为GE计算图。

    **按前端分类的入图路径：**

    | 前端框架 | 入图路径 | 转换机制 |
    | --- | --- | --- |
    | ONNX | ONNX Model → GE Graph | Parser |

    此阶段由框架自动完成，无需开发者提供额外交付件。入图完成后，GE图中已包含自定义算子节点。

4. **阶段4：图编译**

    图编译的回调接口在GE编译期间触发，用于Shape推导、kernel编译和序列化。**回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | InferShape/InferDataType | 编译期Shape/DataType推导 | 所有场景（可选） |
    | Compile | 编译期RTC编译kernel | 在线编译/离线OM场景 |
    | Serialize | ATC编译期写入OM | 离线OM场景 |

    签名、上下文 API、典型流程请参见[回调接口详解](callback_interface.md)。

5. **阶段5：图执行**

    该阶段是自定义算子的核心，GE在执行时回调Execute接口。

    **回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | Execute | 运行时执行kernel | 所有场景（必须实现） |
    | Deserialize | OM加载时恢复binary | 离线OM场景 |
    | UpdateHostArgs | I/O地址变化时刷新 | 静态图下沉场景 |

    回调接口详细示例请参见[回调接口详解](callback_interface.md)。

**总结：ONNX入图阶段与交付件对照表**

| 阶段 | 涉及交付件 | 说明 |
| --- | --- | --- |
| 阶段1：算子定义 | ONNX模型内嵌 | NodeProto自带 |
| 阶段2：前端接入 | framework/onnx/libonnx_parser_plugin.so | 解析插件 |
| 阶段3：图构建入图 | 无 | Parser自动转换 |
| 阶段4：图编译 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |
| 阶段5：图执行 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |

**额外要求**：

- op\_graph/include/\*.h（REG\_OP）供Parser创建节点时使用。
- 解析插件和GE交付件需同时加载。
