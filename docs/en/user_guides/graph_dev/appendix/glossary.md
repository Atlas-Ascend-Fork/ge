# Glossary

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-19T12:29:53.615Z pushedAt=2026-10-21T02:35:24.107Z nl=2 -->

**Table 1** Glossary

|Term|Description|
|--|--|
|Ascend IR|Ascend Intermediate Representation, an abstract data structure dedicated to Ascend AI processors for expressing computation flows. In this document, unless otherwise specified, IR refers to Ascend IR by default.|
|Block|Block has multiple meanings in different scenarios. Generally, it refers to the logical core of an AI Core. Typical scenarios are as follows: AI Core logical core: A block represents a logical core of an AI Core, and its block ID is a logical number starting from 0. Data block: A data block represents a data unit processed by one NPU vector computation instruction. Its size is usually 32 bytes, and one instruction can process multiple data blocks simultaneously. Basic block: It represents the typical data block size required for one computation.|
|Broadcast|Broadcast is a tensor operation mechanism. Through broadcast, a smaller tensor can be automatically expanded to match the shape of a larger tensor.|
|Device|Device refers to the hardware device on which an Ascend AI processor is installed. It connects to the host side through a PCIe interface and provides neural network computing capabilities for the host. Memory resources cannot be shared among multiple devices.|
|Host|It refers to the x86 server or ARM server connected to the device side. It uses the NN (neural network) computing capabilities provided by the device to complete services.|
|Global Memory/GM|The main memory on the device side and the external storage of the AI Core. It is used to store large-scale data, but access patterns need to be optimized to improve performance.|
|Kernel|A kernel function, which is a parallel function executed on the Device.|
|MTE|It refers to the memory transfer engine, the data transfer engine of the AI Core.|
|OP|Operator (OP for short) is a basic unit that performs specific mathematical operations in deep learning algorithms, such as activation functions (for example, ReLU), convolution (Conv), pooling, and normalization (for example, Softmax). By combining these operators, a neural network model can be built.|
|Tiling|Tiling refers to data tiling. When a large amount of data needs to be computed, the data needs to be split across multiple cores, and each core also needs to perform computation in multiple blocks.|
|TilingData|Tiling data refers to the parameters related to data tiling (such as the block size of each transfer and the number of loops). Given the limited Scalar computing capability on the device side, tiling parameters are generally computed on the Host side and then transferred to the device side for use by the kernel function.|
|TilingFunc|The default function provided by the operator project for computing tiling on the Host side.|
|Unified Buffer/UB|The internal storage unit of the AI Core, mainly used for vector computation. It corresponds to the logical memories **AscendC::TPosition::VECIN**, **AscendC::TPosition::VECOUT**, and **AscendC::TPosition::VECCALC**.|
