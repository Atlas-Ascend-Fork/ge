# Prototype Definition Derived APIs

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-16T11:25:35.230Z pushedAt=2026-10-28T07:40:22.177Z nl=2 -->

APIs related to operator prototype definition are automatically generated as corresponding derived APIs, which can be used for IR model construction. For details about how to use the following APIs, see [Graph Development](../../../../user_guides/graph_dev/README.md).

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## REG\_OP

After registering the operator type, two constructors of the operator type are automatically generated.

For example, to register the operator type name **Conv2D**, you can call the **REG\_OP\(Conv2D\)** API. After calling this API, the operator type name **Conv2D** is defined, and two constructors of **Conv2D** are generated, where **Conv2D\(const AscendString& name\)** requires specifying the operator name, and **Conv2D\(\)** uses the default operator name.

```c++
class Conv2D : public Operator {
    typedef Conv2D _THIS_TYPE;
public:
    explicit Conv2D(const char *name);
    explicit Conv2D();
}
```

## INPUT

After the operator input information is registered successfully, the APIs related to the operator input are automatically generated. These APIs are used to obtain the name of the operator input and set the corresponding description of the operator input.

For example, register operator input **x**, whose supported data type is **TensorType\{DT\_FLOAT\}**. You can call the **INPUT\(x, TensorType\{DT\_FLOAT\}\)** API. After the operator input is registered successfully, the following APIs are automatically generated:

```c++
static const string name_in_x(); // Returns the name of the input, that is, "x".
_THIS_TYPE &set_input_x(Operator& v, const string& srcName); // Specifies that input x has a connection relationship with the output srcName of operator object v, and returns the operator object itself.
_THIS_TYPE &set_input_x_by_name(Operator& v, const char *srcName);   // Specifies that input x has a connection relationship with the output srcName of operator object v, and returns the operator object itself.
_THIS_TYPE &set_input_x(Operator &v, uint32_t index); // Specifies that input x has a connection relationship with the output whose index is index of operator object v, and returns the operator object itself.
_THIS_TYPE &set_input_x(Operator& v); // Specifies that input x has a connection relationship with the output whose index is 0 of operator object v, and returns the operator object itself.
TensorDesc get_input_desc_x(); // Returns the description corresponding to input x.
graphStatus update_input_desc_x(const TensorDesc& tensorDesc);// Sets the description corresponding to input x, including Shape, DataType, Format, and other information. graphStatus is of the uint32_t type, and a non-zero return value indicates an error.
```

## OPTIONAL\_INPUT

After the optional operator input information is registered successfully, the related APIs of the operator input are automatically generated, which are used to obtain the name of the operator input, set the corresponding description of the operator input, and so on.

For example, to register operator input b, the data type supported by the operator input is **TensorType\{DT\_FLOAT\}**, and you can call the **OPTIONAL\_INPUT\(b, TensorType\{DT\_FLOAT\}\)** API. After the operator input is registered successfully, the following related APIs are automatically generated:

```c++
static const string name_in_b(); // Returns the name of the input, that is, "b".
_THIS_TYPE& set_input_b(Operator& v, const string& srcName);// Specifies that input b has a connection relationship with the output srcName of operator object v, and returns the operator object itself.
_THIS_TYPE& set_input_b_by_name(Operator& v, const char *srcName);// Specifies that input b has a connection relationship with the output srcName of operator object v, and returns the operator object itself.
_THIS_TYPE& set_input_b(Operator& v); // Specifies that input b has a connection relationship with the output at index 0 of operator object v, and returns the operator object itself.
TensorDesc get_input_desc_b(); // Returns the description corresponding to input b.
graphStatus update_input_desc_b(const TensorDesc& tensorDesc);// Sets the description corresponding to input b, including Shape, DataType, Format, and other information.
```

## DYNAMIC\_INPUT

After the dynamic operator input information is registered successfully, the related APIs for the operator input are automatically generated, which are used to create dynamic inputs, set the corresponding description of the operator input, and so on.

For example, to register the dynamic input d of the operator with the supported data type **TensorType{DT_FLOAT}**, call the **DYNAMIC_INPUT(d, TensorType{DT_FLOAT})** API. After the dynamic input of the operator is registered successfully, the following APIs are automatically generated:

```c++
_THIS_TYPE& create_dynamic_input_d(unsigned int num); // Create the dynamic input d, including num inputs, and use this input as the last input of the operator.
_THIS_TYPE &create_dynamic_input_byindex_d(unsigned int num, size_t index) // Create the dynamic input d, including num inputs, and insert it at the position with the index. This API cannot be used together with create_dynamic_input_d.
TensorDesc get_dynamic_input_desc_d(unsigned int index);// Return the index-th description of the dynamic input d, including Shape, DataType, Format, and other information.
graphStatus update_dynamic_input_desc_d(unsigned int index, const TensorDesc& tensorDesc);// Update the index-th description of the dynamic input d.
_THIS_TYPE& set_dynamic_input_d(unsigned int dstIndex, Operator &v); // Set a connection between the dstIndex-th input of the input d and the output at index 0 of the operator object v, and return the operator object itself.
_THIS_TYPE& set_dynamic_input_d(unsigned int dstIndex, Operator &v, const string &srcName); // Specify a connection between the dstIndex-th input of the dynamic input d and the output srcName of the operator object v, and return the operator object itself.
_THIS_TYPE& set_dynamic_input_d(unsigned int dstIndex, Operator &v, const char *srcName); // Specify a connection between the dstIndex-th input of the dynamic input d and the output srcName of the operator object v, and return the operator object itself.
```

## OUTPUT

After the operator output information is registered successfully, the APIs related to the operator output are automatically generated, which are used to obtain the name of the operator output, obtain the description of the operator output, and set the description of the operator output.

For example, to register operator output y, whose supported data type is **TensorType\{DT\_FLOAT\}**, you can call the **OUTPUT\(y, TensorType\{DT\_FLOAT\}\)** API. After the operator output is registered successfully, the following APIs are automatically generated.

```c++
static const string name_out_y();// Returns the name of the output, that is, "y".
TensorDesc get_output_desc_y();// Returns the description corresponding to output y.
graphStatus update_output_desc_y(const TensorDesc& tensorDesc); // Sets the description corresponding to output y, including Shape, DataType, Format, and other information.
```

## DYNAMIC\_OUTPUT

After the dynamic operator output information is registered successfully, the related APIs for the dynamic operator output are automatically generated, including those used to create dynamic outputs and set the corresponding description of the operator output.

For example, register dynamic operator output d. The data type supported by the operator output is **TensorType\{DT\_FLOAT\}**, and the **DYNAMIC\_OUTPUT\(d, TensorType\{DT\_FLOAT\}\)** API can be called. After the dynamic operator output is registered successfully, the following APIs are automatically generated:

```c++
_THIS_TYPE& create_dynamic_output_d(unsigned int num); // Create dynamic output d, including num outputs.
TensorDesc get_dynamic_output_desc_d(unsigned int index);// Return the index-th description of dynamic output d, including Shape, DataType, Format, and other information.
graphStatus update_dynamic_output_desc_d(unsigned int index, const TensorDesc& tensorDesc);// Update the index-th description of dynamic output d.
```

## REQUIRED\_ATTR

After the operator attribute is registered successfully, three external APIs for the operator attribute are automatically generated, which are used to obtain the name of the attribute, obtain the value of the attribute, and set the value of the attribute.

For example, after registering the attribute mode of type int64\_t, you can call the **REQUIRED\_ATTR\(mode, Int\)** API. After the operator attribute is registered successfully, the following APIs are automatically generated:

```c++
static const string name_attr_mode(); // Returns the name of the attribute, that is, "mode".
static const void name_attr_mode(AscendString &attr_name);// Obtains the name of the attribute through the output parameter, that is, "mode".
OpInt get_attr_mode() const; // Returns the value of the mode attribute. OpInt is int64_t.
_THIS_TYPE& set_attr_mode(const OpInt& v); // Sets the value of the mode attribute and returns the operator object itself.
```

## ATTR

After the operator attribute is registered successfully, three external APIs of the operator attribute are automatically generated, which are used to obtain the attribute name, obtain the attribute value, and set the attribute value.

The following uses two scenarios as examples—an attribute of type int64\_t and a list of type int64\_t—to describe the generated operator attribute APIs:

- Call the **ATTR\(mode, Int, 1\)** API to register the attribute **mode**, whose attribute type is int64\_t and whose default value is **1**.

    After the attribute is registered successfully, the following APIs are automatically generated:

    ```c++
    static const string name_attr_mode(); // Returns the name of the attribute, that is, "mode".
    static const void name_attr_mode(AscendString &attr_name);// Obtains the name of the attribute through the output parameter, that is, "mode".
    OpInt get_attr_mode() const; // Returns the value of the mode attribute. OpInt is int64_t.
    _THIS_TYPE& set_attr_mode(const OpInt& v); // Sets the value of the mode attribute and returns the operator object itself.
    ```

- Call the **ATTR\(pad, ListInt, \{0, 0, 0, 0\}\)** API to register the attribute **pad**, whose attribute type is an int64\_t list and whose default value is **\{0,0,0,0\}**.

    After the attribute is registered successfully, the following APIs are automatically generated:

    ```c++
    static const string name_attr_pad(); // Returns the name of the attribute, that is, "pad".
    static const void name_attr_pad(AscendString &attr_name);// Obtains the name of the attribute, that is, "pad", through an output parameter.
    OpListInt get_attr_pad() const; ; // Returns the value of the attribute pad. OpListInt is vector<int64_t>.
    _THIS_TYPE& set_attr_pad(const OpListInt& v); // Sets the value of the attribute pad and returns the operator object itself.
    ```

The following uses the scenario of registering an attribute of the string type as an example to describe the generated operator attribute APIs:

Call the **ATTR\(data\_format, String, "NHWC"\)** API to register the attribute **data\_format**, whose attribute type is string.

```c++
static const string name_attr_data_format(); // Returns the name of the attribute, that is, "data_format".
static const void name_attr_data_format(AscendString &attr_name);// Obtains the name of the attribute, that is, "data_format", through an output parameter.
OpString get_attr_data_format() const; // Returns the value of the data_format attribute. OpString is string.
graphStatus get_attr_data_format(AscendString &val);// Returns the value of the data_format attribute as an output parameter.
_THIS_TYPE& set_attr_data_format(const string& v); // Sets the value of the data_format attribute and returns the operator object itself.
_THIS_TYPE& set_attr_data_format(const char* v); // Sets the value of the data_format attribute and returns the operator object itself.
```

## GRAPH

After the operator subgraph information is registered successfully, the related APIs of the operator subgraph are automatically generated, allowing users to obtain the name of the operator subgraph, obtain the description of the operator subgraph, and set the description of the operator subgraph.

For example, to register operator subgraph y, you can call the **GRAPH\(y\)** API. After the operator subgraph is registered successfully, the following related APIs are automatically generated:

```c++
static const string name_graph_y();// Returns the name of the operator subgraph, that is, "y".
SubgraphBuilder get_subgraph_builder_y() const;// Returns the builder function object corresponding to subgraph y.
_THIS_TYPE &set_subgraph_builder_y(const SubgraphBuilder &v);// Sets the builder function object corresponding to subgraph y.
Graph get_subgraph_y() const;// Obtains the graph object corresponding to subgraph y.
```

## DYNAMIC\_GRAPH

After the dynamic operator subgraph information is registered successfully, the related APIs of the dynamic operator subgraph are automatically generated, including those used to create dynamic subgraphs and set the corresponding description of the operator subgraph.

For example, to register the dynamic operator subgraph branches, you can call the **DYNAMIC\_GRAPH\(branches\)** API. After the dynamic operator subgraph is registered successfully, the following related APIs are automatically generated:

```c++
_THIS_TYPE& create_dynamic_subgraph_branches(unsigned int num); // Create the dynamic subgraph branches, including num subgraphs.
SubgraphBuilder get_dynamic_subgraph_builder_branches(unsigned int index) ;// Return the builder function object of the index-th subgraph of the dynamic subgraph.
Graph get_dynamic_subgraph_branches(unsigned int index) ;// Return the index-th subgraph object of the dynamic subgraph.
_THIS_TYPE &set_dynamic_subgraph_builder_branches(unsigned int index,const SubgraphBuilder &v);// Set the builder function object of the index-th subgraph of the dynamic subgraph branches.
```
