# Modifying the BatchedMatMul Operator prototxt

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:07:47.904Z pushedAt=2026-10-28T01:47:14.534Z nl=2 -->

> [!NOTE]
>
> None of the code samples in this section can be directly copied into a network model. You need to adjust the corresponding parameters based on the network model you are using. For example, the parameters in bottom and top must correspond one-to-one with the bottom and top in the specific network model, and the order of the parameters corresponding to bottom and top cannot be changed.

This operator is used to output the product of two tensors: y=x1\*x2 (the tensors x1 and x2 are multiplied, and the dimensions of x1 and x2 must be greater than 2 and less than or equal to 8). If your network model uses this operator, see this section, modify the corresponding prototxt, and then perform model conversion.

See the caffe.proto file (the file path is $\{INSTALL\_DIR\}/include/proto). First, add the declaration of the custom layer parameters in the LayerParameter message (the following custom layer has already been declared in caffe.proto, so you do not need to add it again):

```c++
message LayerParameter {
...
  optional BatchedMatMulParameter batched_matmul_param = 235;
...
}
```

See the caffe.proto file. The Type and attributes of this operator are defined as follows:

```c++
message BatchedMatMulParameter{
    optional bool adj_x1 = 1 [default = false];
    optional bool adj_x2 = 2 [default = false];
}
```

See *AI Framework Operator Support List* \> for the list of supported Caffe operators. The BatchedMatMul operator has 2 inputs and 1 output. Based on the preceding principles, the constructed code sample is as follows:

```textproto
layer {
  name: "batchmatmul"
  type: "BatchedMatMul"
  bottom: "matmul_data_1"
  bottom: "matmul_data_2"
  top: "batchmatmul_1"
  batched_matmul_param {
  adj_x1:false
  adj_x2:true
  }
}
```

For parameter descriptions, see *AI Framework Operator Support List* \> Supported Caffe Operators.
