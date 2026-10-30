# Sample Code for Invoking Operators Through the CBLAS API

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-27T16:07:20.423Z pushedAt=2026-10-29T04:02:08.988Z nl=2 -->

This section describes the key APIs and sample code for invoking CBLAS operators in the single-operator model execution mode.

## Basic Principles

For the API invocation process, see [Single Operator Model Execution Overview](single_operator_model_execute_overview.md).

**Currently, the GEMM operator (used for matrix-vector multiplication and matrix-matrix multiplication) has been encapsulated into the aclblas API**, which supports the following two execution modes:

- Execute the operator without a handle. The API name does not contain the "Handle" keyword. For example, call the aclblasGemmEx API (which encapsulates the GEMM operator) to execute the operator.
- Execute the operator with a handle. The API name contains the "Handle" keyword. For example, after calling the aclblasCreateHandleForGemmEx API to create a handle, you also need to call the aclopExecWithHandle API to execute the operator.

>**Note:**
>When the operator is executed without a handle, the system matches the model in memory based on the operator description information each time the operator is executed.
>When the operator is executed with a handle, the system matches the operator description information to the model in memory and caches it in the handle. Each time the operator is executed, there is no need to repeatedly match the operator with the model. Therefore, this mode is more efficient when the same operator is executed multiple times. However, this mode does not support operators with dynamic shapes, and after the handle is no longer used, you need to call the aclopDestroyHandle API to release it.

## Examples

This section uses the aclblasGemmEx API as an example. This API encapsulates the GEMM operator. The matrix multiplication formula in this API is C = αAB + βC, which means that matrix C is obtained by multiplying matrix A and matrix B, where α and β are the coefficients of the product.

Calling the CBLAS API (which encapsulates the GEMM operator) involves the following steps:

1. Prepare the model file of the GEMM operator.
    1. Construct the description file of the GEMM operator (\*.json file, which describes the input and output tensor descriptions, operator attributes, and so on).

        The following is an example of the GEMM operator description file:

        ```bash
        [
        {
          "op": "GEMM",
          "input_desc": [
            {
              "format": "ND",
              "shape": [16, 16],
              "type": "float16"
            },
            {
              "format": "ND",
              "shape": [16, 16],
              "type": "float16"
            },
            {
              "format": "ND",
              "shape": [16, 16],
              "type": "float16"
            },
            {
              "format": "ND",
              "shape": [],
              "type": "float16"
            },
            {
              "format": "ND",
              "shape": [],
              "type": "float16"
            }
          ],
          "output_desc": [
            {
              "format": "ND",
              "shape": [16, 16],
              "type": "float16"
            }
          ],
          "attr": [
          {
            "name": "transpose_a",
            "type": "bool",
            "value": false
          },
          {
            "name": "transpose_b",
            "type": "bool",
            "value": false
            }
          ]
        }
        ]
        ```

    2. Use the ATC tool to compile the operator description file into a single-operator model file (\*.om file), and then call the acl APIs to load the om model file and execute the operator.

        The following is an example of the ATC tool command:

        ```bash
        atc --singleop=$HOME/singleop/gemm.json --output=$HOME/singleop/out/op_model --soc_version=<soc_version>
        ```

        The key parameters are described as follows. For details about parameter values and constraints, see *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*:

        - --singleop: Path of the single-operator description file (in JSON format).
        - --output: Directory for storing the single-operator model file.
        - --soc\_version: Version of the AI processor. _<soc\_version\>_ Replace it based on the actual situation.

2. Write the code logic for calling CBLAS.

    The following code examples show the key steps. They cannot be directly copied, compiled, or run, and are for reference only. After calling the API, add branches for exception handling and record error logs and prompt logs, which are not listed one by one here.

    You can click [gemm](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/1_acl/4_blas/gemm) to obtain the sample.

    ```c++
    // 1. Initialize.
    aclRet = aclInit(nullptr);

    // 2. Apply for runtime resources (use the default Context and default Stream. When the default Stream is used as an input parameter of other APIs, a null pointer can be passed).
    aclRet = aclrtSetDevice(0);
    // Obtain the running mode of the software stack. Different running modes affect the subsequent API calling process (for example, whether data transfer is required).
    aclrtRunMode runMode;
    bool g_isDevice = false;
    aclError aclRet = aclrtGetRunMode(&runMode);
    g_isDevice = (runMode == ACL_DEVICE);

    // 3. Set the directory where the single-operator model file is stored.
    // This directory is relative to the directory where the executable file is located. For example, if the compiled executable file is stored in the run/out directory, this refers to the run/out/op_models directory.
    aclopSetModelDir("op_models");

    // 4. Apply for memory.
    // Apply for memory on the Device to store the input data for operator execution.
    // For this matrix multiplication example, apply for memory to store matrix A data, matrix B data, matrix C data, scalar α data, and scalar β data in sequence.
    aclrtMalloc((void **) &devMatrixA_, sizeA_, ACL_MEM_MALLOC_HUGE_FIRST);
    aclrtMalloc((void **) &devMatrixB_, sizeB_, ACL_MEM_MALLOC_HUGE_FIRST);
    aclrtMalloc((void **) &devMatrixC_, sizeC_, ACL_MEM_MALLOC_HUGE_FIRST);
    aclrtMalloc((void **) &devAlpha_, sizeAlphaBeta_, ACL_MEM_MALLOC_HUGE_FIRST);
    aclrtMalloc((void **) &devBeta_, sizeAlphaBeta_, ACL_MEM_MALLOC_HUGE_FIRST);

    // Apply for memory on the Host. Determine whether Host memory is required based on the running mode of the software stack.
    // If the running mode is ACL_DEVICE, the g_isDevice parameter value is true, indicating that the software stack runs on the Device side. In this case, no Host memory is required, and no image data transfer or intra-Device data transfer is needed.
    // If the running mode is ACL_HOST, the g_isDevice parameter value is false, indicating that the software stack runs on the Host side. In this case, Host memory is required, and data transfer between the Host and Device is involved.
    if (g_isDevice) {
            hostMatrixA_ = devMatrixA_;
            hostMatrixB_ = devMatrixB_;
            hostMatrixC_ = devMatrixC_;
        } else {
            aclrtMallocHost((void **) &hostMatrixA_, sizeA_);
            aclrtMallocHost((void **) &hostMatrixB_, sizeB_);
            aclrtMallocHost((void **) &hostMatrixC_, sizeC_);
        }

    // 5. Prepare the input data. ReadFile is a user-defined function managed by the user, which reads data from a file into memory.
    size_t fileSize;
    // Read matrix A
    char *fileData = ReadFile("test_data/data/matrix_a.bin", fileSize, hostMatrixA_, sizeA_);
    // Read matrix B
    fileData = ReadFile("test_data/data/matrix_b.bin", fileSize, hostMatrixB_, sizeB_);
    // Read matrix C
    fileData = ReadFile("test_data/data/matrix_c.bin", fileSize, hostMatrixC_, sizeC_);
    // Determine whether data transfer between the Host and Device is involved based on the running mode of the software stack.
    if (!g_isDevice) {
        aclError ret = aclrtMemcpy(devMatrixA_, sizeA_, hostMatrixA_, sizeA_, ACL_MEMCPY_HOST_TO_DEVICE);
        ret = aclrtMemcpy(devMatrixB_, sizeB_, hostMatrixB_, sizeB_, ACL_MEMCPY_HOST_TO_DEVICE);
        ret = aclrtMemcpy(devMatrixC_, sizeC_, hostMatrixC_, sizeC_, ACL_MEMCPY_HOST_TO_DEVICE);
    }

    aclrtMemcpyKind kind = g_isDevice ? ACL_MEMCPY_DEVICE_TO_DEVICE : ACL_MEMCPY_HOST_TO_DEVICE;
    ret = aclrtMemcpy(devAlpha_, sizeAlphaBeta_, hostAlpha_, sizeAlphaBeta_, kind);
    ret = aclrtMemcpy(devBeta_, sizeAlphaBeta_, hostBeta_, sizeAlphaBeta_, kind);

    // 6. Execute a single operator.
    // For this example, call the aclblasGemmEx API (an asynchronous API) to implement matrix-matrix multiplication.
    aclblasGemmEx(ACL_TRANS_N, ACL_TRANS_N, ACL_TRANS_N, m_, n_, k_,
                        devAlpha_, devMatrixA_, k_, inputType_, devMatrixB_, n_, inputType_,
                        devBeta_, devMatrixC_, n_, outputType_, ACL_COMPUTE_HIGH_PRECISION,
                        stream);
    // Call the aclrtSynchronizeStream API to block the host until all tasks in the specified stream are complete.
    aclrtSynchronizeStream(nullptr);

    // 7. Transfer the operator execution result, and determine whether data transfer between the host and device is involved based on the running mode of the software stack.
    if (!g_isDevice) {
            auto ret = aclrtMemcpy(hostMatrixC_, sizeC_, devMatrixC_, sizeC_, ACL_MEMCPY_DEVICE_TO_HOST);
    }

    // 8. Whether to directly display the operator execution result on the terminal screen is managed by the user's own code logic.

    // 9. Release runtime resources. The default context and stream do not need to be released by the user; they are automatically released after the aclrtResetDevice API is called.
    aclRet = aclrtResetDevice(0);

    // 10. Deinitialize.
    aclRet = aclFinalize();

    // ......
    ```
