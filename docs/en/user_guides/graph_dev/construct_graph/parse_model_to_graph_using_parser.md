# Using Parser APIs to Parse the Original Model into a Graph

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-21T09:30:05.818Z pushedAt=2026-10-28T03:30:34.491Z nl=2 -->

In addition to constructing graphs directly using operator prototypes, CANN provides a framework parsing capability that parses model formats of mainstream frameworks into the CANN model format.

## Feature Overview

Currently, mainstream open-source deep learning frameworks in the industry (for example, TensorFlow, PyTorch, and Caffe) define models in different formats. For example, TensorFlow describes static-shape graphs and models through custom pb files, while PyTorch describes them through the ONNX specification. Therefore, a unified framework parsing function is required to isolate the differences between upper-layer frameworks. The Parser module completes the parsing and converts the models into the CANN model format supported by AI processors.

![Illustration](../figures/feature_intro_0.png)

The main APIs involved are as follows:

- Parse a TensorFlow model: [aclgrphParseTensorFlow](../../../api/graph_engine_api/cpp/ge/aclgrphParseTensorFlow.md)
- Parse a Caffe model: [aclgrphParseCaffe](../../../api/graph_engine_api/cpp/ge/aclgrphParseCaffe.md)

    <!-- npu="910b" id1 -->
    The Atlas A2 products do not support the Caffe framework: the Caffe framework is no longer evolving in this product form, and its functionality availability is not guaranteed.
    <!-- end id1 -->

    <!-- npu="A3" id2 -->
    The Atlas A3 products do not support the Caffe framework: the Caffe framework is no longer evolving in this product form, and its functionality availability is not guaranteed.
    <!-- end id2 -->

    <!-- npu="950" id3 -->
    The Ascend 950PR&950DT products do not support the Caffe framework: the Caffe framework is no longer evolving in this product form, and its functionality availability is not guaranteed.
    <!-- end id3 -->

    <!-- @ref: ge/res/docs/zh/user_guides/graph_dev/parse_model_to_graph_using_parser_res.md#id1 -->

- Parse an ONNX original model: [aclgrphParseONNX](../../../api/graph_engine_api/cpp/ge/aclgrphParseONNX.md)
- Parse an ONNX model loaded into memory: [aclgrphParseONNXFromMem](../../../api/graph_engine_api/cpp/ge/aclgrphParseONNXFromMem.md)

The Parser layer currently provides users with the capabilities of custom OpParser and custom TensorFlow Scope fusion rules. If users need more flexible adaptation of the framework during Parser parsing, they can customize an OpParser or develop custom TensorFlow Scope fusion rules.

- Custom OpParser:

  If users need to directly map operators in the original framework to Ascend C operators or TBE operators already implemented in CANN, they can directly adapt third-party frameworks. For details, see the "Programming Guide \> Appendix \> AI Framework Operator Adaptation" section in [Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md), or the "Operator Development Process \> Operator Adaptation" section in *[TBE & AI CPU Operator Development](https://hiascend.com/document/redirect/CannCommunityOpdevWizard)*, and select one of the implemented operators.

- Custom TensorFlow Scope fusion rules: A neural network computation graph built based on TensorFlow usually consists of a large number of small operators. To achieve high-performance computation, it is often necessary to fuse the small operators in subgraphs so that the fused large operators can fully utilize hardware acceleration resources. For details, see *TensorFlow Parser Scope Fusion Rule Development*.

>[!NOTE]Note
> <!-- npu="950" id4 -->
>This note **does not apply to** the Ascend 950PR & 950DT products:
> <!-- end id4 -->
>When an original model is converted to a graph, if the number of tensor shape dimensions and format dimensions are inconsistent, interpret the current dimensions according to the rules in the following table:
>For example, if the shape has only one dimension \[16\] and the format has four dimensions, such as NHWC, in this scenario the one dimension of the shape can be interpreted as the C axis, and the other axes need to be padded. After padding, the format is \[1,1,1,16\];
>If the shape has two dimensions \[16,16\] and the format has four dimensions, such as NHWC, in this scenario the two dimensions of the shape can be interpreted as the HW axes, and the other axes need to be padded. After padding, the format is \[1,16,16,1\].

|Actual dimension count|format|Dimension interpretation|
|--|--|--|
|1|NCHWNHWCHWCNCHWNNDHWCNCDHWDHWCNDHWNC|C|
|2|NCHW|CH|
|2|NHWC|HW|
|2|HWCN|CN|
|2|CHWN|WN|
|2|NDHWC|WC|
|2|NCDHW|HW|
|2|DHWCN|CN|
|2|DHWNC|NC|
|3|NCHW|CHW|
|3|NHWC|HWC|
|3|HWCN|WCN|
|3|CHWN|HWN|
|3|NDHWC|HWC|
|3|NCDHW|DHW|
|3|DHWCN|WCN|
|3|DHWNC|WNC|
|4|NDHWC|DHWC|
|4|NCDHW|CDHW|
|4|DHWCN|HWCN|
|4|DHWNC|HWNC|

## TensorFlow Model Parsing

Included headers:

```c++
#include "tensorflow_parser.h"
```

The [aclgrphParseTensorFlow](../../../api/graph_engine_api/cpp/ge/aclgrphParseTensorFlow.md) API is used to parse the original TensorFlow model as a graph. At this point, the graph is stored in the memory buffer.

```c++
std::string tfPath = "../data/tf_test.pb";
ge::Graph graph1;
auto tfStatus = ge::aclgrphParseTensorFlow(tfPath.c_str(),graph1);
```

Simultaneously, users can specify **parser\_params**:

```c++
std::string tfPath = "../data/tf_test.pb";
std::map<ge::AscendString, ge::AscendString> parser_params = {
            {ge::AscendString(ge::ir_option::INPUT_FP16_NODES), ge::AscendString("input1;input2")},
            {ge::AscendString(ge::ir_option::OUTPUT), ge::AscendString("newIssue")}};
ge::Graph graph1;
auto tfStatus = ge::aclgrphParseTensorFlow(tfPath.c_str(), parser_params, graph1);
```

## Caffe Model Parsing

Included headers:

```c++
#include "caffe_parser.h"
```

The [aclgrphParseCaffe](../../../api/graph_engine_api/cpp/ge/aclgrphParseCaffe.md) API parses the original Caffe model into a graph, which is stored in the memory buffer at this point.

```c++
std::string caffePath = "../data/caffe_test.prototxt";
std::string weight = "../data/caffe_test.caffemodel";
ge::Graph graph1;
auto caffeStatus = ge::aclgrphParseCaffe(caffePath.c_str(), weight.c_str(), graph1);
```

Simultaneously, users can specify **parser\_params:**

```c++
std::string caffePath = "../data/caffe_test.prototxt";
std::string weight = "../data/caffe_test.caffemodel";
std::map<ge::AscendString, ge::AscendString> parser_params = {
            {ge::AscendString(ge::ir_option::INPUT_FP16_NODES), ge::AscendString("input1;input2")},
            {ge::AscendString(ge::ir_option::OUTPUT), ge::AscendString("newIssue")}};
ge::Graph graph1;
auto caffeStatus = ge::aclgrphParseCaffe(caffePath.c_str(), weight.c_str(), parser_params, graph1);
```

## Parsing an ONNX Model

Included header files:

```c++
#include "onnx_parser.h"
```

Use [aclgrphParseONNXFromMem](../../../api/graph_engine_api/cpp/ge/aclgrphParseONNXFromMem.md) to parse the ONNX model loaded into memory as a graph. At this point, the graph is stored in the memory buffer. Simultaneously, the API supports users to specify **parser\_params:**

```c++
// Open an ONNX model file in binary mode.
FILE *pFile = fopen("./onnx/resnet101.onnx", "rb" );
if(pFile==NULL)
{
    fputs("File error",stderr);
    exit(1);
}

// Read the file size.
fseek(pFile, 0, SEEK_END);
long lSize = ftell(pFile);
rewind(pFile);

// Allocate a memory buffer of sufficient size.
char *buffer =(char*) malloc(sizeof(char)*lSize);
if(buffer == NULL)
{
    fputs("Memory error", stderr);
    exit(2);
}
// Read the file content into the buffer.
size_t result = fread(buffer, 1, lSize, pFile);
if(result != lSize)
{
    fputs("Reading error", stderr);
    exit(3);
}
// Prepare the parameters required for ONNX parsing.
std::map<ge::AscendString, ge::AscendString> parser_params= {
            {ge::AscendString(ge::ir_option::INPUT_FP16_NODES), ge::AscendString("input1;input2")},
            {ge::AscendString(ge::ir_option::OUTPUT), ge::AscendString("newIssue")}};
// Parse the ONNX model.
ge::Graph graph1;
auto onnxStatus = ge::aclgrphParseONNXFromMem(buffer, result, parser_params, graph1);
```

Use [aclgrphParseONNX](../../../api/graph_engine_api/cpp/ge/aclgrphParseONNX.md) to parse the original ONNX model as a graph. At this point, the graph is stored in the memory buffer. Simultaneously, the API allows users to specify **parser\_params:**

```c++
std::string onnxPath = "../data/onnx_test.onnx";
std::map<ge::AscendString, ge::AscendString> parser_params= {
            {ge::AscendString(ge::ir_option::INPUT_FP16_NODES), ge::AscendString("input1;input2")},
            {ge::AscendString(ge::ir_option::OUTPUT), ge::AscendString("newIssue")}};
ge::Graph graph1;
auto onnxStatus = ge::aclgrphParseONNX(onnxPath.c_str(), parser_params, graph1);
```
