# Overall Description of Single Operator Model Execution

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-27T16:14:27.568Z pushedAt=2026-10-29T07:56:38.354Z nl=2 -->

**Note: The single operator model execution API in this chapter will be deprecated in later versions. For the replacement scheme, see [Deprecated API Replacement Scheme](deprecated_APIs_replacement_scheme.md).**

**Single operator model execution refers to executing operators based on graph IR (Intermediate Representation)**. First, use the Ascend Tensor Compiler (ATC) to compile the single operator description file defined by Ascend IR into an operator om model file. Then call the acl API to load the operator model (for example, the [aclopSetModelDir](aclopSetModelDir.md) API). Finally, call the acl API to execute the operator (for example, the [aclopExecuteV2](aclopExecuteV2.md) API).

**Figure 1**  Single operator model execution API call flow
![](figures/single_operator_model_API_calling_process.png "single operator model API calling process")

The key APIs are described as follows (APIs starting with aclrt are provided by the Runtime component):

1. **Compile the operator.**

    Based on how the operator is compiled, there are two modes:

    - After the operator is compiled, operator-related data is stored in the \*.om model file.

        To compile the operator in this mode, use the ATC tool. For details, see *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*. Compile the single operator definition file (\*.json) into an offline model (\*.om file) adapted to the AI processor.

        After the operator is compiled, perform steps 2, 3, 4, 5, 6, and 7 in sequence.

    - After the operator is compiled, the operator-related data is stored in memory.

        To compile an operator in this mode, call the acl API. Call different APIs for different scenarios:

        - For the scenario where the same operator is compiled once and executed multiple times, call the aclopCompile API to compile the operator. After the operator is compiled, perform steps 3, 4, 5, 6, and 7 in sequence.
        - For the scenario where the operator is compiled and executed the same number of times, perform step 3 first, and then call the aclopCompileAndExecute API to compile the operator. After the operator is compiled, perform steps 6 and 7 in sequence.

2. **Load the operator model file**.

    Load the single operator model file in either of the following two ways:

    - Call the aclopSetModelDir API to set the directory where the model file is loaded. The single operator model file (\*.om file) is stored in this directory.
    - Call the aclopLoad API to load the single operator model data from memory, with the memory managed by you. The single operator model data refers to the data obtained by compiling the single operator into a \*.om file and then reading the om file into memory.

3. Call the aclrtMalloc API to **allocate memory on the Device** for storing the input and output data of the operator to be executed.

    If data on the Host needs to be transferred to the Device, call the aclrtMemcpy API (synchronous API) or the aclrtMemcpyAsync API (asynchronous API) to transfer the data through memory copy.

4. In dynamic shape scenarios, if the output shape of the operator cannot be determined, **derive or estimate the output shape of the operator** before executing the operator.

    You need to call APIs such as aclopInferShape, aclGetTensorDescNumDims, aclGetTensorDescDimV2, and aclGetTensorDescDimRange to derive or estimate the output shape of the operator, which serves as the input to the operator execution API aclopExecuteV2.

5. **Execute the operator**.

    - For operators encapsulated as acl APIs, including the GEMM operator (see the CBLAS API) and the Cast operator, the following two execution modes are supported:
        - Execute the operator not based on a handle. The API name does not contain the "Handle" keyword. For example, call the aclblasGemmEx API (encapsulating the GEMM operator) or the aclopCast API (encapsulating the Cast operator) to execute the operator.
        - Execute the operator based on a handle. The API name contains the "Handle" keyword. For example, after calling the aclblasCreateHandleForGemmEx API or the aclopCreateHandleForCast API to create a handle, you also need to call the aclopExecWithHandle API to execute the operator.

    - For operators not encapsulated as acl APIs, the following two execution modes are supported:
        - Execute the operator not based on a handle by calling the aclopExecuteV2 API.
        - Execute the operator based on a handle by calling the aclopCreateHandle API to create a handle, and then call the aclopExecWithHandle API to execute the operator.

    >**Note:**
    >When the operator is executed not based on a handle, the system internally matches the model in memory based on the operator description information each time the operator is executed.
    >When the operator is executed based on a handle, the system internally matches the operator description information to the model in memory and caches it in the handle. Each time the operator is executed, there is no need to repeatedly match the operator with the model. Therefore, this mode is more efficient when the same operator is executed multiple times. However, this mode does not support dynamic shape operators, and after the handle is no longer used, you need to call the aclopDestroyHandle API to release it.

6. Call the aclrtSynchronizeStream API to **block the application** until all tasks in the specified stream are complete.
7. Call the aclrtFree API to **release the memory**.

    If you need to transfer the operator execution result data on the Device to the Host, call the aclrtMemcpy API (synchronous API) or the aclrtMemcpyAsync API (asynchronous API) to transfer data through memory copy, and then release the memory.
