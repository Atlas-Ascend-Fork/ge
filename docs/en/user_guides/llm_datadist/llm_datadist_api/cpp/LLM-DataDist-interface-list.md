# LLM-DataDist-interface-list

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:56:22.945Z pushedAt=2026-11-17T03:39:12.106Z nl=2 -->

LLM-DataDist is a large model distributed cluster and data acceleration component that provides cluster KV data management capabilities to support the separate deployment of full graphs and incremental graphs.

The supported product forms are as follows:

- Atlas A2 inference product series
- Atlas A3 products

The LLM-DataDist V1 C++ API is planned to be deprecated, and the related header files will no longer be installed with ge_compiler. To use the LLM-DataDist header files, install toolkit and hixl.

The library file corresponding to the API is libllm\_engine.so.

The LLM-DataDist interface list is as follows.

**Table 1** LLM-DataDist interfaces

| Interface Name | Overview |
| --- | --- |
| [LLMDataDist-constructor](LlmDataDist-constructor.md)|Creates an LLM-DataDist object. |
| [~LlmDataDist-destructor](LlmDataDist-destructor.md) | Destructor of the LLM-DataDist object. |
| [Initialize](Initialize.md) | Initializes LLM-DataDist. |
| [Finalize](Finalize.md) | Releases LLM-DataDist. |
| [SetRole](SetRole.md) | Sets the role of the current LLM-DataDist. |
| [LinkLlmClusters](LinkLlmClusters.md) | Establishes links. |
| [UnlinkLlmClusters](UnlinkLlmClusters.md) | Disconnects links. |
| [PullKvCache](PullKvCache.md) | Pulls KV Cache in contiguous memory mode. |
| [PullKvBlocks](PullKvBlocks.md) | Pulls KV Cache by block list. |
| [CopyKvCache](CopyKvCache.md) | Copies KV Cache in contiguous memory mode. |
| [CopyKvBlocks](CopyKvBlocks.md) | Copies KV Cache by block list. |
| [PushKvCache](PushKvCache.md) | Pushes Cache to the remote node. This API can be called only when the role is Prompt. |
| [PushKvBlocks](PushKvBlocks.md) | Pushes Cache to the remote node by block list. This API can be called only when the role is Prompt. |
| [AllocateCache](AllocateCache.md) | Allocates Cache. |
| [DeallocateCache](DeallocateCache.md) | Releases Cache. |
