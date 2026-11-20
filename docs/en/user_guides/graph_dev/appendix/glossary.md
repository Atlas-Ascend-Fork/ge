# Glossary

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-06T10:28:25.732Z pushedAt=2026-11-12T06:47:03.920Z nl=2 -->

**Table 1** Glossary

|Glossary|Description|
|--|--|
|Ascend IR|Ascend Intermediate Representation, an abstract data structure dedicated to Ascend AI Processors for expressing computation flows. In this document, unless otherwise specified, IR refers to Ascend IR by default.|
|Block|Block has multiple meanings in different scenarios. Generally, it refers to the logical core of an AI Core. Typical scenarios are as follows: AI Core logical core: A Block represents a logical core of an AI Core, and its BlockID is a logical number starting from 0. DataBlock: A DataBlock represents the data unit processed by one NPU vector computation instruction. Its size is usually 32 bytes, and one instruction can process multiple DataBlocks simultaneously. Basic block: It represents the typical data block size required for one computation.|
|Broadcast|Broadcast is a tensor operation mechanism. Through broadcasting, a smaller tensor can be automatically expanded to match the shape of a larger tensor.|
|Device|Device refers to the hardware device on which an Ascend AI Processor is installed. It connects to the host side through a PCIe interface and provides neural network computing capabilities for the host. If multiple Devices exist, memory resources cannot be shared among them.|
|Host|Host refers to the X86 server or ARM server connected to the Device side. It uses the NN (Neural-Network) computing capabilities provided by the Device to complete services.|
|Global Memory/GM|The main memory on the device side and the external storage of the AI Core. It is used to store large-scale data, but access patterns need to be optimized to improve performance.|
|Kernel|A kernel function, which is a parallel function executed on the Device.|
|MTE|Memory Transfer Engine, the data transfer engine of the AI Core.|
|OP|Operator (OP for short), the basic unit that performs a specific mathematical operation in a deep learning algorithm, such as an activation function (for example, ReLU), convolution (Conv), pooling, and normalization (for example, Softmax). A neural network model can be built by combining these operators.|
|Tiling|Tiling refers to the splitting and blocking of data. When the amount of data to be computed is large, the data needs to be split across multiple cores, and each core also needs to perform computation in multiple blocks.|
|TilingData|TilingData refers to the parameters related to data splitting and blocking, such as the block size of each transfer and the number of loops. Given the limited Scalar computing capability on the device side, Tiling parameters are generally computed on the Host side and then transferred to the device side for use by the Kernel function.|
|TilingFunc|The default function provided by the operator project for computing Tiling on the Host side.|
|Unified Buffer/UB|The internal storage unit of the AI Core, mainly used for vector computation. It corresponds to the logical memories **AscendC::TPosition::VECIN**, **AscendC::TPosition::VECOUT**, and **AscendC::TPosition::VECCALC**.|
