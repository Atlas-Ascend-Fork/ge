# LLM-DataDist-interface-list

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:20:20.651Z pushedAt=2026-10-29T06:08:55.152Z nl=2 -->

LLM-DataDist: a large model distributed cluster and data acceleration component that provides cluster KV data management capabilities and supports separate deployment of full graphs and incremental graphs.

The supported product forms are as follows:

- Atlas A2 inference products
- Atlas A3 products

The LLM-DataDist V1 C++ APIs are planned to be deprecated, and the related header files will no longer be installed with ge_compiler. To use the LLM-DataDist header files, install toolkit and hixl.

The library file corresponding to the APIs is libllm\_engine.so.

The LLM-DataDist interface list is as follows.

**Table 1**  LLM-DataDist APIs

| Interface Name | Overview |
| --- | --- |
| [LLMDataDist-constructor](LlmDataDist-constructor.md)|Creates an LLM-DataDist object. |
| [~LlmDataDist-destructor](LlmDataDist-destructor.md) | Destructor of the LLM-DataDist object. |
| [Initialize](Initialize.md) | Initializes LLM-DataDist. |
| [Finalize](Finalize.md) | Releases LLM-DataDist. |
| [SetRole](SetRole.md) | Sets the role of the current LLM-DataDist. |
| [LinkLlmClusters](LinkLlmClusters.md) | Establishes links. |
| [UnlinkLlmClusters](UnlinkLlmClusters.md) | Disconnects links. |
| [PullKvCache](PullKvCache.md) | Pulls the KV Cache in contiguous memory mode. |
| [PullKvBlocks](PullKvBlocks.md) | Pulls the KV Cache in block list mode. |
| [CopyKvCache](CopyKvCache.md) | Copies the KV Cache in contiguous memory mode. |
| [CopyKvBlocks](CopyKvBlocks.md) | Copies the KV Cache in block list mode. |
| [PushKvCache](PushKvCache.md) | Pushes the cache to a remote node. This API can be called only when the role is Prompt. |
| [PushKvBlocks](PushKvBlocks.md) | Pushes the cache to a remote node in block list mode. This API can be called only when the role is Prompt. |
| [AllocateCache](AllocateCache.md) | Allocates the cache. |
| [DeallocateCache](DeallocateCache.md) | Releases the cache. |
