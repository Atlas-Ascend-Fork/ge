# Expression Examples of Operators in a Graph

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T09:26:58.180Z pushedAt=2026-10-28T03:30:34.487Z nl=2 -->

## Defining a Data Node (Data)

The input node of a graph, that is, the data node, is implemented by using the Data operator.

The Data operator prototype is defined as follows:

```c++
REG_OP(Data)
    .INPUT(x, TensorType::ALL())
    .OUTPUT(y, TensorType::ALL())
    .ATTR(index, Int, 0)
    .OP_END_FACTORY_REG(Data)
```

Create a Data operator instance named data based on the Data operator prototype definition, with the initialization parameter desc\_data. At the same time, set Shape, Format, and Dtype through the “update\_input\_desc\_input name" and “update\_output\_desc\_output name" interface settings, keeping them consistent with the data information that the user needs to process.

```c++
auto shape_data = vector<int64_t>({1,17,2,2});
TensorDesc desc_data(ge::Shape(shape_data), FORMAT_ND, DT_FLOAT);
auto data = op::Data("data");                // Create a Data operator.
data.update_input_desc_x(desc_data);         // Set the operator input description
data.update_output_desc_y(desc_data);        // Set the operator output description.
```

Note that when defining a data node, you **must** set Shape, Format, and Dtype through the “update\_input\_desc\_ input name" and “update\_output\_desc\_ output name" interface settings.

## Defining a Constant Node (Const)

Weights, biases, and similar information are constant tensors and can be implemented through the Const operator.

The Const operator prototype is defined as follows:

```c++
REG_OP(Const)
    .OUTPUT(y, TensorType({DT_FLOAT, DT_FLOAT16, DT_BF16, DT_INT4, DT_INT8, DT_INT16, DT_UINT16, \
        DT_UINT8, DT_INT32, DT_INT64, DT_UINT32, DT_UINT64, DT_BOOL, DT_DOUBLE}))
    .ATTR(value, Tensor, Tensor())
    .OP_END_FACTORY_REG(Const)
```

### Directly Constructing Weight Data

Create a Const operator instance based on the Const operator prototype definition, with the initial value (that is, the value of the value attribute) set to weighttensor1.

```c++
// Construct weighttensor1
TensorDesc weight_desc(ge::Shape({1,3,3,3}), FORMAT_NCHW, DT_INT8);
int bs_size_weight = 27;
int8_t * bs_inputData_weight = nullptr;
bs_inputData_weight = new int8_t[bs_size_weight];
for (int i=0; i<bs_size_weight; ++i) {
    *(bs_inputData_weight+i) = 1;
}
Tensor weighttensor1(weight_desc, (uint8_t*)bs_inputData_weight, bs_size_weight*sizeof(int8_t));

// Create a Const operator, with the initial value (that is, the value of the value attribute) set to weighttensor1
auto weight1 = op::Const().set_attr_value(weighttensor1);
```

If the prototype input and output of an operator have the same name, the operator is an inplace operator, meaning that its output updates its input. In this scenario, such an input cannot be connected to a Const node.

### Reading Weight Data from a File

In addition to directly constructing weight data, you can also read weight data directly from a bin file.

```c++
// Construct weight_tensor
auto weight_shape = ge::Shape({ 5,17,1,1 });
TensorDesc desc_weight_1(weight_shape, FORMAT_NCHW, DT_INT8);
Tensor weight_tensor(desc_weight_1);
uint32_t weight_1_len = weight_shape.GetShapeSize();
// "const_0.bin" is the path of the constant file
GetConstTensorFromBin("const_0.bin", weight_tensor, weight_1_len*sizeof(int8_t));

// Create a Const operator whose initial value (that is, the value of the value attribute) is weight_tensor
auto conv_weight = op::Const("const_0").set_attr_value(weight_tensor);
```

Implementation of the GetConstTensorFromBin function:

```c++
bool GetConstTensorFromBin(string path, Tensor &weight, uint32_t len) {
    // Open the file in binary mode
    ifstream in_file(path.c_str(), std::ios::in | std::ios::binary);
    if(!in_file.is_open()) {
        std::cout << "failed to open" << path.c_str() << "\n";
        return false;
    }
    // Move the file pointer to the end of the file to obtain the total file size
    in_file.seekg(0,  std::ios_base::end);
    std::istream::pos_type file_size = in_file.tellg();
    in_file.seekg(0, ios_base::beg);  // Move the file pointer back to the beginning of the file

    // Check whether the len specified by the user matches the actual file size
    if(len != file_size) {
        std::cout << "Invalid Param.len:" << len << " is not equal with binary size(" << file_size << ")\n";
        in_file.close();
        return false;
    }
    // Allocate a memory buffer for the file content
    char* pdata = new(std::nothrow) char[len];
    if(pdata == nullptr) {
        std::cout << "Invalid Param.len:" << len << " is not equal with binary size(" << file_size << ")\n";
        in_file.close();
        return false;
    }
    // Read data from the file into the buffer
    in_file.read(reinterpret_cast<char*>(pdata), len);
    // Set the data in the buffer to the Tensor object weight
    auto status = weight.SetData(reinterpret_cast<uint8_t*>(pdata), len);
    if(status != ge::GRAPH_SUCCESS) {
        std::cout << "Set Tensor Data Failed"<< "\n";
        in_file.close();
        return false;
    }
    in_file.close();
    delete[] pdata;
    return true;
}
```

Parameter description of the GetConstTensorFromBin function:

- path: input parameter, specifies the weight file path, used to search for the weight file xx.bin in a fixed directory such as "../data/weight/". The user needs to parse the weight file into a bin file.
- weight: output parameter, the Tensor-type weight data read from the weight file.
- len: input parameter, specifies the size of the weight data.

## Defining a Required Input Operator (SoftmaxV2)

The following uses SoftmaxV2 as an example to describe how to define an operator.

SoftmaxV2 operator prototype definition:

```c++
REG_OP(SoftmaxV2)
    .INPUT(x, TensorType({ DT_DOUBLE, DT_FLOAT16, DT_BF16, DT_FLOAT }))
    .OUTPUT(y, TensorType({ DT_DOUBLE, DT_FLOAT16, DT_BF16, DT_FLOAT }))
    .ATTR(axes, ListInt, {-1})
    .ATTR(half_to_float, Bool, false)
    .OP_END_FACTORY_REG(SoftmaxV2)
```

As shown in the SoftmaxV2 operator prototype, the SoftmaxV2 operator has one required input whose input name is x. Create a SoftmaxV2 operator instance:

```c++
auto softmax = op::SoftmaxV2("Softmax")     // Create an operator instance and pass the operator name (for example, Softmax) as an input parameter
    .set_input_x(matmul2);                  // Set the operator input (for example, matmul2)
```

## Defining an Optional Input Operator (Conv2D)

The following uses Conv2D as an example to describe how to define an operator.

The Conv2D operator prototype is defined as follows:

```c++
REG_OP(Conv2D)
    .INPUT(x, TensorType({DT_FLOAT16, DT_FLOAT, DT_INT8, DT_BF16}))
    .INPUT(filter, TensorType({DT_FLOAT16, DT_FLOAT, DT_INT8, DT_BF16}))
    .OPTIONAL_INPUT(bias, TensorType({DT_FLOAT16, DT_FLOAT, DT_INT32}))
    .OPTIONAL_INPUT(offset_w, TensorType({DT_INT8}))
    .OUTPUT(y, TensorType({DT_FLOAT16, DT_FLOAT, DT_INT32, DT_BF16}))
    .REQUIRED_ATTR(strides, ListInt)
    .REQUIRED_ATTR(pads, ListInt)
    .ATTR(dilations, ListInt, {1, 1, 1, 1})
    .ATTR(groups, Int, 1)
    .ATTR(data_format, String, "NHWC")
    .ATTR(offset_x, Int, 0)
    .OP_END_FACTORY_REG(Conv2D)
```

As shown in the Conv2D operator prototype, the Conv2D operator includes two required inputs x and filter, two optional inputs bias and offset\_w, two required attributes strides and pads, and four optional attributes dilations, groups, data\_format, and offset\_x. Based on the preceding definition, the code for defining the Conv2D operator is as follows:

```c++
auto conv2d = op::Conv2D("Conv2d")
    // quant, conv_weight, and conv_bias are three input nodes
    .set_input_x(quant)
    .set_input_filter(conv_weight)
    .set_input_bias(conv_bias)
    .set_attr_strides({ 1, 1, 1, 1 })
    .set_attr_pads({ 0, 0, 0, 0 })
    .set_attr_dilations({ 1, 1, 1, 1 });

TensorDesc conv2d_input_desc_x(ge::Shape(), FORMAT_NCHW, DT_INT8);
TensorDesc conv2d_input_desc_filter(ge::Shape(), FORMAT_NCHW, DT_INT8);
TensorDesc conv2d_input_desc_bias(ge::Shape(), FORMAT_NCHW, DT_INT32);
TensorDesc conv2d_output_desc_y(ge::Shape(), FORMAT_NCHW, DT_INT32);
conv2d.update_input_desc_x(conv2d_input_desc_x);
conv2d.update_input_desc_filter(conv2d_input_desc_filter);
conv2d.update_input_desc_bias(conv2d_input_desc_bias);
conv2d.update_output_desc_y(conv2d_output_desc_y);
```

The main process is as follows:

1. Call the operator type constructor “Conv2D\(const char\* name\)” to create an operator instance and pass the operator name (for example, Conv2d) as an input parameter.

    ```c++
    auto conv2d1 = op::Conv2D("Conv2d")
    ```

2. Call the “set\_input\__input name_" interface to set the operator input.

    ```c++
        .set_input_x(data)
        .set_input_filter(conv_weight)
        .set_input_bias(conv_bias)
    ```

    **data** is the input node of the entire graph, constructed by the Data operator. For details, see [Defining a Data Node (Data)](#defining-a-data-node-data).

    **conv\_weight** is constant data, constructed by the Const operator. For details, see [Defining a Constant Node (Const)](#defining-a-constant-node-const).

    **conv\_bias** is constant data, constructed through the Const operator. For details, see [Defining a Constant Node (Const)](#defining-a-constant-node-const).

3. Call the “set\_attr\__attribute name_" interface to set the operator attributes.

    ```c++
    .set_attr_strides({1, 1, 1, 1})       // Set the strides attribute value.
    .set_attr_pads({0, 0, 0, 0})          // Set the pads attribute value.
    .set_attr_dilations({1, 1, 1, 1});    // Set the dilations attribute value.
    ```

4. For convolution operators such as Conv2D or operators sensitive to C-axis processing, it is recommended to set the Format information to NCHW or NHWC through the “update\_input\_desc\_input name" interface, keeping it consistent with the Format that the user needs to process.

    ```c++
    TensorDesc conv2d_input_desc_x(ge::Shape(), FORMAT_NCHW, DT_INT8);
    TensorDesc conv2d_input_desc_filter(ge::Shape(), FORMAT_NCHW, DT_INT8);
    TensorDesc conv2d_input_desc_bias(ge::Shape(), FORMAT_NCHW, DT_INT32);
    TensorDesc conv2d_output_desc_y(ge::Shape(), FORMAT_NCHW, DT_INT32);
    conv2d.update_input_desc_x(conv2d_input_desc_x);
    conv2d.update_input_desc_filter(conv2d_input_desc_filter);
    conv2d.update_input_desc_bias(conv2d_input_desc_bias);
    conv2d.update_output_desc_y(conv2d_output_desc_y);
    ```

    IR graph construction does not support the following FORMATs:

    ```c++
    FORMAT_NC1HWC0
    FORMAT_FRACTAL_Z
    FORMAT_NC1C0HWPAD
    FORMAT_NHWC1C0
    FORMAT_FRACTAL_DECONV
    FORMAT_C1HWNC0
    FORMAT_FRACTAL_DECONV_TRANSPOSE
    FORMAT_FRACTAL_DECONV_SP_STRIDE_TRANS
    FORMAT_NC1HWC0_C04
    FORMAT_FRACTAL_Z_C04
    FORMAT_FRACTAL_DECONV_SP_STRIDE8_TRANS
    FORMAT_NC1KHKWHWC0
    FORMAT_C1HWNCoC0
    FORMAT_FRACTAL_ZZ
    FORMAT_FRACTAL_NZ
    FORMAT_NDC1HWC0
    FORMAT_FRACTAL_Z_3D
    FORMAT_FRACTAL_Z_3D_TRANSPOSE
    FORMAT_FRACTAL_ZN_LSTM
    FORMAT_FRACTAL_Z_G
    FORMAT_ND_RNN_BIAS
    FORMAT_FRACTAL_ZN_RNN
    FORMAT_NYUV
    FORMAT_NYUV_A
    ```

## Defining a Dynamic Multi-Input Operator (AddN)

Some operators have a variable number of inputs and are dynamic multi-input operators, for example, AddN. The following describes how to define such an operator.

AddN operator prototype definition:

```c++
REG_OP(AddN)
    .DYNAMIC_INPUT(x, TensorType({NumberType(), DT_VARIANT}))
    .OUTPUT(y, TensorType({NumberType(), DT_VARIANT}))
    .REQUIRED_ATTR(N, Int)
    .OP_END_FACTORY_REG(AddN)
```

As shown in the AddN operator prototype definition, this operator is a dynamic multi-input operator. We create dynamic inputs through “create\_dynamic\_input\__input name_", and set dynamic inputs through “set\_dynamic\_input\__input name_".

```c++
auto data = op::Data().set_attr_index(0);
auto addn = op::AddN("addn")
 .create_dynamic_input_x(2)    // Create dynamic input x, including two inputs, and use these two inputs as the last inputs of the operator.
 .set_dynamic_input_x(0,data)     // Set the first input. 0 indicates the input index, which starts from 0 by default, and data indicates the input value.
 .set_dynamic_input_x(1,data)     // Set the second input. 1 indicates the input index, which starts from 0 by default, and data indicates the input value.
 .set_attr_N(2);                  // Set the value of attribute N to 2, indicating that the operator has two inputs.
```

You can also create dynamic inputs through “create\_dynamic\_input\_byindex\__input name_", but it cannot be used together with “create\_dynamic\_input\__input name_". The difference between the two is that “create\_dynamic\_input\__input name_" uses the created dynamic inputs as the last inputs of the operator by default, while “create\_dynamic\_input\_byindex\__input name_" can specify the index position of the dynamic inputs, for example:

```c++
auto addn = op::AddN("addn")
 .create_dynamic_input_byindex_x(2,0)    // Create dynamic input x, including two inputs, and insert these two inputs at index 0 and index 1 (where 0 indicates the start index of the dynamic inputs).
 .set_dynamic_input_x(0,data1)                // Set the first input. 0 indicates the input index, which starts from 0 by default. data1 indicates the input value.
 .set_dynamic_input_x(1,data2)                // Set the second input. 1 indicates the input index, which starts from 0 by default. data2 indicates the input value.
 .set_attr_N(2);                              // Set the value of attribute N to 2, indicating that the operator has two inputs.
```

## Defining a Dynamic Multi-Output Operator (Split)

Some operators have a variable number of outputs and are dynamic multi-output operators, for example, Split. The following describes how to define such an operator.

Split operator prototype definition:

```c++
REG_OP(Split)
    .INPUT(split_dim, TensorType({DT_INT32}))
    .INPUT(x, TensorType({DT_COMPLEX128, DT_COMPLEX64, DT_DOUBLE, DT_FLOAT,  DT_FLOAT16, DT_INT16,
                          DT_INT32,      DT_INT64,     DT_INT8,   DT_QINT16, DT_QINT32,  DT_QINT8,
                          DT_QUINT16,    DT_QUINT8,    DT_UINT16, DT_UINT32, DT_UINT64,  DT_UINT8,
                          DT_BF16,       DT_BOOL}))
    .DYNAMIC_OUTPUT(y, TensorType({DT_COMPLEX128, DT_COMPLEX64, DT_DOUBLE, DT_FLOAT,  DT_FLOAT16, DT_INT16,
                                   DT_INT32,      DT_INT64,     DT_INT8,   DT_QINT16, DT_QINT32,  DT_QINT8,
                                   DT_QUINT16,    DT_QUINT8,    DT_UINT16, DT_UINT32, DT_UINT64,  DT_UINT8,
                                   DT_BF16,       DT_BOOL}))
    .REQUIRED_ATTR(num_split, Int)
    .OP_END_FACTORY_REG(Split)

```

As shown in the Split operator prototype definition, this operator is a dynamic multi-output operator. We create dynamic outputs through the “create\_dynamic\_output\__output name_.

```c++
auto split = op::Split("split")
  .set_input_x(data)                         // Construct a Data operator.
  .set_input_split_dim(const)                // Construct a Const operator.
  .set_attr_num_split(2)
  .create_dynamic_output_y(2);           // Create dynamic output y of the split operator, including two outputs.

auto addn = op::AddN("addn")
  .create_dynamic_input_x(1)                 // Create dynamic input x, including one input.
  .set_dynamic_input_x(0, split, "y0")       // Set the first input of the addn operator. split indicates the input operator, and y0 indicates the output name of the split operator, where y0 is the first output.
  .set_attr_N(1);                            // Set the value of attribute N to 1, indicating that the operator has 1 input

auto softplus = op::Softplus("softplus")
  .set_input_x(split, "y1");                 // Set the input of the softplus operator, where split indicates the input operator and y1 indicates the second output of the split operator
```

For details about how to construct a Data operator, see [Defining a Data Node (Data)](#defining-a-data-node-data). For details about how to construct a Const operator, see [Defining a Constant Node (Const)](#defining-a-constant-node-const).

## Defining the Data Type Conversion Operator (Cast)

When building a graph through operator prototypes, the dtypes of adjacent operators must be consistent. If the output dtype of the previous operator does not match the input dtype of the next operator, a Cast operator must be inserted.

For example, in the following sample, the AddN operator requires float32 input, but the output of the Greater operator is of the bool type. When the data type changes, a Cast operator must be inserted to perform data type conversion.

![](../figures/define_dtype_convert_op.png)

Cast operator prototype definition:

```c++
REG_OP(Cast)
    .INPUT(x, TensorType({DT_BOOL, DT_FLOAT16, DT_FLOAT, DT_INT8, DT_INT32, DT_UINT32, DT_UINT8,
                          DT_INT64, DT_UINT64, DT_INT16, DT_UINT16, DT_DOUBLE, DT_COMPLEX64,
                          DT_COMPLEX128, DT_QINT8, DT_QUINT8, DT_QINT16, DT_QUINT16, DT_QINT32, DT_BF16, DT_UINT1}))
    .OUTPUT(y, TensorType({DT_BOOL, DT_FLOAT16, DT_FLOAT, DT_INT8, DT_INT32, DT_UINT32, DT_UINT8,
                           DT_INT64, DT_UINT64, DT_INT16, DT_UINT16, DT_DOUBLE, DT_COMPLEX64,
                           DT_COMPLEX128, DT_QINT8, DT_QUINT8, DT_QINT16, DT_QUINT16, DT_QINT32,
                           DT_BF16, DT_COMPLEX32}))
    .REQUIRED_ATTR(dst_type, Int)
    .OP_END_FACTORY_REG(Cast)
```

From the Cast operator prototype definition, you can see that there is a required attribute dst\_type, which indicates the data type after conversion. Setting it to 0 indicates that the data type after conversion is float32. For the mapping between values and data types, see the "ge namespace \>DataType" in [Basic Data Structures and APIs](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

```c++
auto greater = op::Greater("greater").set_input_x1(const1).set_input_x2(const2);
auto cast = op::Cast("cast").set_input_x(greater)
                            .set_attr_dst_type(0);
auto addn = op::AddN("addn").create_dynamic_input_x(3)
                           .set_dynamic_input_x(0,cast)
                           .set_dynamic_input_x(1,data).set_dynamic_input_x(2,data)
                           .set_attr_N(3);
```
