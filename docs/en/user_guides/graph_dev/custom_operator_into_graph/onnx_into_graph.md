# ONNX Graph Integration

<!-- md-trans-meta sourceCommit=557fc263a581ed94fc4bb5447e90358eea64338d translatedAt=2026-11-06T11:12:53.401Z pushedAt=2026-11-13T11:14:23.210Z nl=2 -->

The ONNX frontend differs from PyTorch: the GE ONNX Parser directly reports an error for op types it does not recognize (PARAM\_INVALID). Therefore, an additional **ONNX parsing plugin** is required to map ONNX node attributes to GE operator attributes. A brief diagram of the deliverables on both sides is shown below:

![](../figures/onnx_into_graph.png)

1. **Stage 1: Operator Definition**

    In the operator definition stage, metadata such as the inputs, outputs, and attributes of an operator is declared for use in subsequent stages.

    **Implementation by frontend:**

    | Frontend Framework | Definition Method | Deliverable | Description |
    | --- | --- | --- | --- |
    | ONNX | NodeProto | ONNX model embedding | The model only declares the op semantics; the REG_OP header file on the GE side still needs to be written manually.<br> Note: The NodeProto embedded in the ONNX model is only the "usage declaration" of the op in the model. The GE side still requires op_graph/include/*.h (REG_OP) for the Parser to use when creating nodes. The two serve different purposes and cannot replace each other. |

2. **Stage 2: Frontend Integration (Parsing Plugin)**

    Map the frontend operator type to the GE operator type and handle attribute conversion.

    **Integration method by frontend:**

    | Frontend Framework | Integration method | Deliverables Involved | Key Point |
    | --- | --- | --- | --- |
    | ONNX | Parser parsing plugin | libonnx_parser_plugin.so | ParseParamsFn must be implemented manually |

    **ONNX operator type identifier format**: "domain::version::OpType" (for example, "ai.onnx::11::Conv" and "com.example::1::MyOp"). The Parser searches the OpRegistry for a mapping in this format. If no mapping is found, parsing fails.

    **The ONNX parsing plugin must:**

    1. Use REGISTER\_CUSTOM\_OP to register the mapping from ONNX op type to GE op type.
    2. Implement ParseParamsFn to extract attributes from the ONNX NodeProto and set them to the GE Operator.

    **Parsing plugin code:**

    ```c++
    // framework/onnx/add_custom_parser.cpp
    #include "register/register.h"
    #include "proto/onnx/ge_onnx.pb.h"

    // Attribute parsing function: maps ONNX NodeProto attributes to GE Operator attributes.
    // op_src: ONNX node Proto message body.
    // op_dest: GE operator object, used to set attributes.
    static domi::Status ParseMyOp(const google::protobuf::Message *op_src, ge::Operator &op_dest) {
      // Convert the Message to the ONNX NodeProto type.
      auto *node = dynamic_cast<const ge::onnx::NodeProto *>(op_src);
      // Traverse all attributes of the ONNX node.
      for (const auto &attr : node->attribute()) {
        // Set the alpha attribute in ONNX to the GE operator.
        if (attr.name() == "alpha") {
          op_dest.SetAttr("alpha", attr.f());
        }
      }
      return domi::SUCCESS;
    }

    // Register the ONNX parsing plugin: maps the ONNX op type to the GE op type.
    // When the ONNX Parser encounters an unrecognized operator, it looks up the parsing plugin through this registration information.
    REGISTER_CUSTOM_OP("MyCustomOp")
        .FrameworkType(domi::ONNX)                                    // Declare this as an ONNX framework plugin.
        .OriginOpType({"ai.onnx::11::MyOp", "ai.onnx::13::MyOp"})     // ONNX operator type (multiple versions supported)
        .ParseParamsFn(ParseMyOp);                                    // Attribute parsing callback function

    ```

    **REGISTER\_CUSTOM\_OP Key methods:**

    | Method | Purpose |
    | --- | --- |
    | .FrameworkType(domi::ONNX) | Declares this as an ONNX framework plugin. |
    | .OriginOpType("domain::ver::Type") | Specifies the ONNX op type (supports a list, covering multiple versions). |
    | .ParseParamsFn(fn) | Attribute parsing function (Message* → Operator&). |
    | .ParseParamsByOperatorFn(fn) | Attribute parsing on the automatic mapping path (Operator → Operator). |
    | .ParseOpToGraphFn(fn) | Expands a single operator into a subgraph (optional). |
    | .ParseSubgraphPostFn(fn) | Post-processing for operators containing subgraphs (optional). |

    The ONNX scenario requires two .so files:

    | Deliverable | Responsibility | Placement path |
    | --- | --- | --- |
    | ONNX parsing plugin .so | REGISTER_CUSTOM_OP + ParseParamsFn, responsible for ONNX → GE attribute mapping | Plugin directory under ASCEND_CUSTOM_OPP_PATH |
    | GE custom operator .so | REG_OP + REG_AUTO_MAPPING_OP + execution logic | op_graph/lib/\<os>/\<arch>/ |

    **Deliverable organization example:**

    ```tree
    custom_opp/
    ├── op_graph/
    │   ├── include/add_custom.h    # REG_OP
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE deliverable
    ├── framework/
    │   └── onnx/
    │       └── libonnx_parser_plugin.so       # Parsing plugin (standalone .so)
    ```

    For the unified deliverable specifications, see [Deliverables and OPP Specifications](deliverables_and_opp.md).

    **Note:**

    - The ONNX parsing plugin .so must be loaded through ASCEND\_CUSTOM\_OPP\_PATH before ATC compilation; otherwise, the Parser cannot recognize the custom op type.
    - When the domain in OriginOpType is empty, "ai.onnx" is used by default. A custom domain must be explicitly specified.
    - If an ONNX operator can be mapped to a combination of existing GE operators, it can be expanded into a subgraph through ParseOpToGraphFn, without the need to write a GE-side custom operator .so.

3. **Stage 3: Graph construction and integration**

    In the graph construction and integration stage, the frontend computation graph is converted into a GE computation graph.

    **Graph construction paths by frontend:**

    | Frontend Framework | Graph input path | Conversion mechanism |
    | --- | --- | --- |
    | ONNX | ONNX Model → GE Graph | Parser |

    This stage is automatically completed by the framework, and no additional deliverables are required from developers. After the graph input is completed, the GE graph already contains the custom operator nodes.

4. **Stage 4: Graph Compilation**

    The callback interfaces for graph compilation are triggered during GE compilation and are used for shape inference, kernel compilation, and serialization. **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenario |
    | --- | --- | --- |
    | InferShape/InferDataType | Shape/DataType inference at compile time | All scenarios (optional) |
    | Compile | RTC kernel compilation at compile time | Online compilation/offline OM scenario |
    | Serialize | Writing to OM during ATC compilation | Offline OM scenario |

    For the signature, context APIs, and typical process, see [Callback Interface](callback_interface.md).

5. **Stage 5: Graph Execution**

    This stage is the core of a custom operator. GE calls back the Execute interface during execution.

    **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenario |
    | --- | --- | --- |
    | Execute | Kernel execution at runtime | All scenarios (must be implemented) |
    | Deserialize | Restoring binary when OM is loaded | Offline OM scenario |
    | UpdateHostArgs | Refreshing when I/O addresses change | Static graph offloading scenario |

    For detailed examples of the callback interfaces, see [Callback Interface](callback_interface.md).

**Summary: ONNX graph construction stages and deliverables involved**

| Stage | Deliverables Involved | Description |
| --- | --- | --- |
| Stage 1: operator definition | ONNX model embedding | Provided by NodeProto |
| Stage 2: frontend integration | framework/onnx/libonnx_parser_plugin.so | Parsing plugin |
| Stage 3: graph construction and integration | None | Automatically converted by the Parser |
| Stage 4: graph compilation | op_graph/lib/<os\>/<arch\>/*.so | Callback implemented in GE deliverables |
| Stage 5: graph execution | op_graph/lib/<os\>/<arch\>/*.so | Callback implemented in GE deliverables |

**Additional requirements:**

- op\_graph/include/\*.h (REG\_OP) is used by the Parser when creating nodes.
- The parsing plugin and the GE deliverable must be loaded simultaneously.
