# Example Code for Executing Dynamic Shape Operators

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:09:22.615Z pushedAt=2026-10-29T04:05:29.007Z nl=2 -->

This section describes the key APIs and example code for invoking dynamic shape operators based on single-operator model execution.

## Basic Principles

For operators that support dynamic shapes:

- If the output shape of an operator is explicit, the basic execution process of such operators is similar to that of fixed-shape operators. For the API call process, see [Single Operator Model Execution Overview](single_operator_model_execute_overview.md). For sample code for executing fixed-shape operators, see [Sample Code for Executing Fixed-Shape Operators](static_shape_op_exec_sample_code.md).
- If the output shape of an operator cannot be determined, before calling aclopExecuteV2, you need to call aclopInferShape, aclGetTensorDescNumDims, aclGetTensorDescDimV2, aclGetTensorDescDimRange, and other APIs to infer or estimate the output shape of the operator, which is then used as the input to the operator execution API aclopExecuteV2.

    For the aclopInferShape API, the memory for the operator input tensor data must be determined based on the application running mode. If the application runs on the Host, apply for memory on the Host. If the application runs on the Device, apply for memory on the Device.

## Sample Code

The following code shows the key steps. It is for reference only and cannot be directly copied, compiled, and run. After calling the API, add exception handling branches and record error and prompt logs, which are not listed here one by one.

```c++
// ......

const char *opType;
int numInputs;
aclTensorDesc *inputDesc[2];
aclDataBuffer *inputs[2];
int numOutputs;
aclTensorDesc *outputDesc[1];
aclopAttr *attr;

aclError ret = aclopInferShape(opType, numInputs, inputDesc, inputs, numOutputs, outputDesc, attr);

std::vector<std::vector<int64_t>> tensorDims; // Shape of the output tensor after inferShape.
// Iterate over each output of the operator to infer or estimate the shape value:
for (int index = 0; index < numOutputs; ++index) {
    std::vector<int64_t> dimSize; // Indicates the output shape when the operator is executed.
    size_t dimNums = aclGetTensorDescNumDims(outputDesc[index]);
        // Indicates that the number of dimensions is unknown in the dynamic shape scenario. Reserved for this scenario.
    if (dimNums == ACL_UNKNOWN_RANK)  {
        // The maximum shape value is estimated by the user.
        dimSize.push_back(max_shape);
    } else {
        for (size_t i = 0; i < dimNums; ++i) {
            int64_t dim;
            ret = aclGetTensorDescDimV2(outputDesc[index], i, &dim);

                        // Indicates that the dimension value is dynamic in the dynamic shape scenario.
            if(dim == -1) {
                int64_t dimRange[2];
                                // Obtain the shape range and use the maximum shape value in the range to construct the output tensorDesc as the input of aclopExecuteV2.
                ret = aclGetTensorDescDimRange(outputDesc[index], i, 2, dimRange);
                dim = dimRange[1];
            }
            dimSize.push_back(dim);
        }
    }
    tensorDims.push_back(dimSize);
}

// Construct the operator input tensorDesc and input tensor as the input of aclopExecuteV2.
aclTensorDesc *inputDescNew[2];
aclDataBuffer *inputsNew[2];
aclDataBuffer *outputsNew[1];
// The preceding section provides the output Shape during operator execution. Construct the output tensorDesc (that is, the outputDescNew parameter value) based on the dims in tensorDims, and use it to call aclopExecuteV2.
ret = aclopExecuteV2(opType, numInputs, inputDescNew, inputsNew, numOutputs, outputDescNew, outputsNew, attr, stream);

// For the preceding scenario where the user estimates the Shape value and uses the maximum Shape in the Shape range, add the following call after operator execution to obtain the accurate Shape:
// Iterate over the tensorDesc of each output in a for loop.
std::vector<std::vector<int64_t>> outTensorDims; // Accurate output tensorShape.
for (int index = 0; index < numOutputs; ++index) {
    std::vector<int64_t> dimSize;
    int dimNums = aclGetTensorDescNumDims(outputDescNew[index]);
    for (int i = 0; i < dimNums; i++){
        int64_t dim;
        ret = aclGetTensorDescDimV2(outputDescNew[index], i, &dim);
        dimSize.push_back(dim);
    }
    outTensorDims.push_back(dimSize);
}
// ......
```
