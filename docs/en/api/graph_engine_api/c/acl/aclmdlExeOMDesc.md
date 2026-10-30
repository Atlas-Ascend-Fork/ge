# aclmdlExeOMDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T15:08:06.771Z pushedAt=2026-10-28T13:15:59.560Z nl=2 -->

```c
typedef struct aclmdlExeOMDesc {
    size_t workSize;
    size_t weightSize;
    size_t modelDescSize;
    size_t kernelSize;
    size_t kernelArgsSize;
    size_t staticTaskSize;
    size_t dynamicTaskSize;
    size_t fifoTaskSize;
    size_t reserved[8];
} aclmdlExeOMDesc;
```

| Member Name | Description |
| --- | --- |
| workSize | Size of the work memory required for model execution, in bytes. |
| weightSize | Size of the weight memory required for model execution, in bytes. |
| modelDescSize | Memory size required to store the model description, in bytes. |
| kernelSize | Memory size required to store the TBE operator kernel (the *.o and*.json files of the operator), in bytes. |
| kernelArgsSize | Memory size required to store the TBE operator kernel parameters, in bytes. |
| staticTaskSize | Memory size required to store the static shape task description, in bytes. |
| dynamicTaskSize | Memory size required to store the dynamic shape task description, in bytes. |
| fifoTaskSize | Size of the model-level global memory, in bytes.<br>If, during inference, the input of each layer of a model is formed by concatenating the output of the previous layer with the inference results of several preceding rounds, the model-level global memory is required to save the input data needed by the model for subsequent inference. |
| reserved | Reserved value. |
