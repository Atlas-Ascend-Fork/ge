# Dump Graph Details

<!-- md-trans-meta sourceCommit=f21fcdd0483aafceaf30c4bff43b317d1a83ef94 translatedAt=2026-10-27T14:30:42.715Z pushedAt=2026-10-28T10:28:20.697Z nl=2 -->

Before model conversion, set the following environment variables to dump the model graphs generated during compilation. You can view the dumped graphs to observe model changes:

```bash
export DUMP_GE_GRAPH=1      # Control the amount of content in the dumped graphs
export DUMP_GRAPH_LEVEL=1   # Control the number of dumped graphs
```

The following files are generated in the current directory where the atc command is executed. For details about the environment variables, see [2](../overview/environment_setup.md).

- ge\_onnx\*.pbtxt: ONNX-based model description structure, which can be opened using visualization tools such as Netron.
- ge\_proto\*.txt: text file stored in protobuf format. This file can be converted to a JSON file to help you locate issues. This file usually appears in pairs with ge\_onnx\*.pbtxt, but ge\_proto\*.txt contains additional string-type attribute information compared with ge\_onnx\*.pbtxt. Therefore, ge\_proto\*.txt displays more complete information. You can open either of the two files.

    Because the ge\_proto\*.txt file structure has been optimized for file size compared with ge\_onnx\*.pbtxt, setting the DUMP\_GE\_GRAPH environment variable to 2 or 3 has the same effect on the ge\_proto\*.txt file, both displaying a basic dump that does not contain data such as weights.

- ge\_readable\*.txt: (Optional) highly readable text file similar to the Dynamo fx graph style. This type of file is generated only when the DUMP\_GRAPH\_FORMAT environment variable is set and its value includes "readable".

Each of the preceding files corresponds to a step in the model compilation process. For example, the process starts with ge\_onnx\_xxxx\_PreRunBegin.pbtxt and ends with ge\_onnx\_xxxx\_PreRunAfterBuild.pbtxt. Each file contains all operators involved in completing the corresponding step. For details about the functions of the subgraphs at each stage of the dumped graphs, see the following table. (The dumped subgraphs generated for each model may differ, but the main process is basically the same.)

**Table 1** Detailed description of dump graph information

| Sequence Number | Subgraph Name | Involved Components | Stage Description | Whether Trustlist |
| --- | --- | --- | --- | --- |
| 1 | ge_proto_xxxx_GraphPreRunBegin.txt | GE | Graph before Graph compilation | - |
| 2 | ge_proto_xxxx_AfterFlowGraphPartition.txt | GE | Graph after Flow partitioning (Flow partitioning is a partitioning method applied in DataFlow) | - |
| 3 | ge_proto_xxxx_AfterParallelPartitioner.txt | GE | Graph after pipeline parallel partitioning (pipeline here refers to PP in the backend inference scenario) | - |
| 4 | ge_proto_xxxx_PreRunBegin.txt | GE | Original graph structure | Trustlist |
| 5 | ge_proto_xxxx_RunCustomPassBeforeInfershape.txt | GE | Exit graph of the user-defined Pass before InferShape | Trustlist |
| 6 | ge_proto_xxxx_PreRunAfterInitPreparation.txt | FE | Graph structure after all initialization processing in the graph preparation stage | - |
| 7 | ge_proto_xxxx_PreRunAfterHandleSummaryOp.txt | GE | Graph structure after processing Summary nodes | - |
| 8 | ge_proto_xxxx_PrepareAfterCheckAndUpdateInput.txt | GE | Graph structure after validating and updating graph input data processing | - |
| 9 | ge_proto_xxxx_PrepareAfterGraphEquivalentTransformation.txt | GE | Graph structure after equivalently replacing the For loop graph structure with the While loop graph structure | - |
| 10 | ge_proto_xxxx_PrepareAfterProcessOutput.txt | GE | Graph structure after related processing of graph data | - |
| 11 | ge_proto_xxxx_PrepareAfterOptimizeAfterGraphNormalization.txt | GE | Exit graph of the graph optimization operation after graph normalization | - |
| 12 | ge_proto_xxxx_PrepareAfterInsertAipp.txt | GE | Graph structure after AIPP-related processing of the graph when AIPP parameters are configured | - |
| 13 | ge_proto_xxxx_PrepareAfterProcessAippNodesDataFormat.txt | GE | Exit graph of AIPP node format refresh | - |
| 14 | ge_proto_xxxx_PreRunAfterNormalizeGraph.txt | GE | Exit graph of graph normalization | Trustlist |
| 15 | ge_proto_xxxx_PreRunAfterOptimizeGraphInit.txt | GE | Exit graph of graph optimization initialization | - |
| 16 | ge_proto_xxxx_OptimizeGraph_TagNoConstFoldingAfter.txt | FE | Used in quantization scenarios. FE tags operators as not to perform constant folding. GE checks this tag when performing constant folding and skips constant folding if the tag exists | - |
| 17 | ge_proto_xxxx_HcclAfterOptimizeGraphPrepare.txt | HCCL | Graph after optimization in the HCCL graph preparation stage | - |
| 18 | ge_proto_xxxx_PreRunAfterOptimizeGraphPrepare.txt | GE | Graph structure after original graph preparation processing through each operator information library (OptimizeGraphPrepare interface call) | - |
| 19 | ge_proto_xxxx_PrepareAfterProcessBeforeInfershape.txt | GE | Graph structure after dead-edge elimination processing of conditional operators | - |
| 20 | ge_proto_xxxx_AfterFirstInferformat.txt | GE | Graph structure after whole-graph inferformat processing | - |
| 21 | ge_proto_xxxx_AfterInfershape.txt | GE | Graph structure after whole-graph infershape processing, accompanied by constant folding | Trustlist |
| 22 | ge_proto_xxxx_PrepareAfterInferFormatAndShape.txt | GE | Graph structure after all InferFormat and InferShape processing. A second whole-graph InferFormat is performed between this graph and the previous one | - |
| 23 | ge_proto_xxxx_RunCustomPass_AfterInferShape.txt | GE | Exit graph of the user-defined Pass after InferShape | Trustlist |
| 24 | ge_proto_xxxx_AfterSecondInferformat.txt | GE | Graph after the second format inference | - |
| 25 | ge_proto_xxxx_PrepareAfterCtrlFlowPreProcess.txt | GE | Graph structure after preprocessing of conditional operators | - |
| 26 | ge_proto_xxxx_PrepareAfterGetDynamicOutputShape.txt | GE | Graph structure after processing graph output in dynamic profile scenarios | - |
| 27 | ge_proto_xxxx_PrepareAfterProcessAippStage2.txt | GE | Graph structure after related processing of graph input nodes in AIPP mode | - |
| 28 | ge_proto_xxxx_PrepareAfterPrepareOptimize.txt | GE | Graph structure after related optimization processing in the graph preparation stage | - |
| 29 | ge_proto_xxxx_PreRunAfterPrepare.txt | GE | Currently the same as the previous graph. Graph structure after all graph preparation processing | Trustlist |
| 30 | ge_proto_xxxx_OptimizeQuantGraph_FeGraphFusionAfter.txt | FE | Graph structure after the quantization process in the graph optimization stage | - |
| 31 | ge_proto_xxxx_OptimizeOriginalGraph_FeGraphFusionAfter.txt | FE | Graph structure after the graph fusion process | - |
| 32 | ge_proto_xxxx_OptimizeOriginalGraph_FeTopoSortingAfter.txt | FE | Graph structure after topological sorting following graph fusion, used to check whether a cycle is formed after fusion | - |
| 33 | ge_proto_xxxx_OptimizeOriginalGraph_DSAFeOpJudgeAfter.txt | FE | Graph after DataType and Format selection following dynamic shape analysis | - |
| 34 | ge_proto_xxxx_HcclBeforeOptimizeOriginalGraph.txt | HCCL | Graph before HCCL original graph optimization | - |
| 35 | ge_proto_xxxx_HcclAfterOptimizeOriginalGraph.txt | HCCL | Graph after HCCL original graph optimization | - |
| 36 | ge_proto_xxxx_RunCustomPassAfterBuiltinFusionPass.txt | GE | Exit graph of the user-defined Pass after built-in Passes | Trustlist |
| 37 | ge_proto_xxxx_PreRunAfterOptimizeOriginalGraph.txt | GE | Graph structure after original graph optimization processing through each operator information library (OptimizeOriginalGraph interface call) | Trustlist |
| 38 | ge_proto_xxxx_PrepareAfterUpdateInputOutputByUserOptions.txt | GE | Graph structure after related processing of graph input and output based on user parameters | - |
| 39 | ge_proto_xxxx_PrepareAfterUpdateVariableFormats.txt | GE | Graph structure after related processing of variable Formats | - |
| 40 | ge_proto_xxxx_PreRunAfterPrepareRunningFormatRefiner.txt | GE | Same as the previous graph | - |
| 41 | ge_proto_xxxx_BeforeOptimizeOriginalGraphJudgeInsert.txt | FE | Entry graph of the op_judge process | - |
| 42 | ge_proto_xxxx_OptimizeOriginalGraph_FeOpDtypeJudgeAfter.txt | FE | Graph after precision mode selection | - |
| 43 | ge_proto_xxxx_PreRunAfterRefineRunningPrecision.txt | GE | Graph after precision selection | - |
| 44 | ge_proto_xxxx_AfterPrecisionRefine.txt | GE | Graph generated after conversion operator fusion following precision selection | - |
| 45 | ge_proto_xxxx_PreRunAfterAfterPrecisionRefine.txt | GE | Graph generated after conversion operator fusion and then automatic fusion following precision selection<br>If automatic fusion is not enabled, the content of this subgraph is the same as that of the<br>ge_proto_xxxx_AfterPrecisionRefine.txt subgraph | - |
| 46 | ge_proto_xxxx_OptimizeOriginalGraph_FeOpFormatJudgeAfter.txt | FE | Graph of the complete op_judge after format selection | - |
| 47 | ge_proto_xxxx_OptimizeOriginalGraph_FeDistHeavyFormatAfter.txt | FE | Graph structure after heavy operator diffusion | - |
| 48 | ge_proto_xxxx_OptimizeOriginalGraph_FeInsertTransNodeAfter.txt | FE | Graph structure after inserting conversion operators | - |
| 49 | ge_proto_xxxx_PreRunAfterRefineRunningFormat.txt | GE | Graph structure after optimization processing through each operator information library (OptimizeOriginalGraphJudgeInsert interface call) | - |
| 50 | ge_proto_xxxx_PreRunAfterSubexpressionMigration.txt | GE | Graph structure after common subexpression extraction in dynamic profile scenarios | - |
| 51 | ge_proto_xxxx_before_SameTransdataBreadthFusionPass.txt | GE | Entry graph of SameTransdataBreadthFusionPass | - |
| 52 | ge_proto_xxxx_after_SameTransdataBreadthFusionPass.txt | GE | Exit graph of SameTransdataBreadthFusionPass | - |
| 53 | ge_proto_xxxx_OptimizeStage1_1.txt | GE | Graph structure after graph optimization stage 1_1 processing | - |
| 54 | ge_proto_xxxx_OptimizeStage1_2.txt | GE | Graph structure after graph optimization stage 1_2 processing | - |
| 55 | ge_proto_xxxx_PreRunAfterOptimize1.txt | GE | Graph structure after all graph optimization stage 1 processing | - |
| 56 | ge_proto_xxxx_PreRunAfterOptimizeAfterStage1.txt | GE | Graph structure after optimization processing through each operator information library (OptimizeAfterStage1 interface call) | Trustlist |
| 57 | ge_proto_xxxx_RunCustomPassAfterOriginGraphOptimize.txt | GE | Executes the custom pass after the original graph optimization stage | Trustlist |
| 58 | ge_proto_xxxx_PreRunAfterInferShape2.txt | GE | Graph structure after the second InferShape processing | - |
| 59 | ge_proto_xxxx_BeforeStagePartition.txt | GE | Graph before Stage partitioning | - |
| 60 | ge_proto_xxxx_AfterStagePartition.txt | GE | Graph after Stage partitioning | - |
| 61 | ge_proto_xxxx_AfterEnginePlacer.txt | GE | Graph after engine selection is completed | - |
| 62 | ge_proto_xxxx_Before_DSP.txt | GE | Graph before dynamic/static model splitting | - |
| 63 | ge_proto_xxxx_After_DSP.txt | GE | Graph after dynamic/static model splitting | - |
| 64 | ge_proto_xxxx_AfterDynamicShapePartition.txt | GE | Graph structure after dynamic shape graph splitting | - |
| 65 | ge_proto_xxxx_MergedComputeGraphAfterCompositeEnginePartition.txt | GE | Merged graph structure after independent subgraph splitting and subgraph optimization processing | - |
| 66 | ge_proto_xxxx_partition0_rank0_inputNodeGraph_AtomicEnginePartitioning.txt | GE | Graph structure of the input node subgraph after atomic engine rule-based graph splitting | - |
| 67 | ge_proto_xxxx_partition0_rank1_new_sub_graph1_AtomicEnginePartitioning.txt | GE | Graph structure of subgraph 1 after atomic engine rule-based graph splitting | - |
| 68 | ge_proto_xxxx_partition0_rank2_new_sub_graph110_AtomicEnginePartitioning.txt | GE | Graph structure of subgraph 110 after atomic engine rule-based graph splitting | - |
| 69 | ge_proto_xxxx_OptimizeSubgraphPreProc.txt | GE | Exit graph of subgraph optimization preprocessing | - |
| 70 | ge_proto_xxxx_DNN_VM_RTS_OptimizeSubGraphBefore.txt | RTS | - | - |
| 71 | ge_proto_xxxx_DNN_VM_RTS_OptimizeSubGraphAfter.txt | RTS | - | - |
| 72 | ge_proto_xxxx_AIcoreEngine_OptimizeSubGraphBefore.txt | FE | Entry graph of AI Core subgraph optimization | - |
| 73 | ge_proto_xxxx_OptimizeSubGraphBefore.txt | GE | Subgraph structure before the subgraph optimization operation. Each subgraph has one copy with the same name but a different sequence number. The total number is determined by the number of subgraphs | - |
| 74 | ge_proto_xxxx_OptimizeSubGraphAfter.txt | GE | Subgraph structure after the subgraph optimization operation. Each subgraph has one copy with the same name but a different sequence number. The total number is determined by the number of subgraphs | - |
| 75 | ge_proto_xxxx_partition0_rank1_new_sub_graph1_lxfusion_input.txt | AOE(lxfusion) | sgat input graph in the ATC scenario and AOE baseline scenario | - |
| 76 | ge_proto_xxxx_partition0_rank1_new_sub_graph1_after_rebuild.txt | AOE(lxfusion) | UB fusion graph in the internal AOE sgat process | - |
| 77 | ge_proto_xxxx_AIcoreEngine_OptimizeSubGraphAfter.txt | FE | Exit graph of AI Core subgraph optimization | - |
| 78 | ge_proto_xxxx_OptimizeSubgraphPostProc.txt | GE | Exit graph of subgraph optimization post-processing | - |
| 79 | ge_proto_xxxx_mergedComputeGraph.txt | GE | Graph structure after graph merging, same as the previous graph | - |
| 80 | ge_proto_xxxx_MergedComputeGraphAfterAtomicEnginePartition.txt | GE | Merged graph structure after independent atomic engine splitting and subgraph optimization processing | - |
| 81 | ge_proto_xxxx_PreRunAfterOptimizeSubgraph.txt | GE | Graph structure after subgraph optimization processing | Trustlist |
| 82 | ge_proto_xxxx_OptimizeWholeGraphaicpu_tf_optimizer.txt | GE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 83 | ge_proto_xxxx_OptimizeWholeGraphaicpu_ascend_optimizer.txt | GE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 84 | ge_proto_xxxx_OptimizeWholeGraphdvpp_graph_optimizer.txt | GE | Exit graph after whole-graph DVPP optimization | - |
| 85 | ge_proto_xxxx_OptimizeWholeGraphhccl_alltoallvc_fusion_optimizer.txt | HCCL | Graph after fusion optimization in the HCCL original graph optimization stage | - |
| 86 | ge_proto_xxxx_OptimizeWholeGraphAIcoreEngine.txt | GE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 87 | ge_proto_xxxx_OptimizeWholeGraphDSAEngine.txt | FE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 88 | ge_proto_xxxx_OptimizeWholeGraphhccl_graph_optimizer.txt | HCCL | Graph optimized in the HCCL original graph optimization stage | - |
| 89 | ge_proto_xxxx_OptimizeWholeGraphDNN_VM_RTS_GRAPH_OPTIMIZER_STORE.txt | GE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 90 | ge_proto_xxxx_OptimizeWholeGraphDNN_VM_RTS_FFTS_PLUS_GRAPH_OPTIMIZER_STORE.txt | RTS | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 91 | ge_proto_xxxx_OptimizeWholeGraphDNN_VM_HOST_CPU_OPTIMIZER.txt | GE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 92 | ge_proto_xxxx_OptimizeWholeGraphge_local_graph_optimizer.txt | GE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 93 | ge_proto_xxxx_OptimizeWholeGraphffts_plus.txt | FE | Graph information after calling the original graph optimization interface of each engine. The part after OptimizeWholeGraph is the engine name | - |
| 94 | ge_proto_xxxx_PreRunAfterOptimizeWholeGraph.txt | GE | Graph structure after optimization processing through each operator information library (OptimizeWholeGraph interface call) | - |
| 95 | ge_proto_xxxx_PreRunAfterOptimize2.txt | GE | Graph structure after all graph optimization stage 2 processing | - |
| 96 | ge_proto_xxxx_PreRunAfterOptimizeGraphBeforeBuild.txt | GE | Entry graph of model compilation | Trustlist |
| 97 | ge_proto_xxxx_PreRunAfterOptimizeTensorMove.txt | GE | Graph after optimizing redundant TensorMove nodes | - |
| 98 | ge_proto_xxxx_BeforeHandleMemConflict.txt | GE | Graph before memory conflict handling | - |
| 99 | ge_proto_xxxx_AfterHandleMemConflict.txt | GE | Graph after memory conflict handling | - |
| 100 | ge_proto_xxxx_BeforeHandleMemoryLayoutConflict.txt | GE | Entry graph for resolving memory layout conflicts | - |
| 101 | ge_proto_xxxx_PreRunAfterMemConflictProc.txt | GE | Exit graph for resolving memory read/write conflicts | - |
| 102 | ge_proto_xxxx_partition0_rank0_inputNodeGraph_SecondPartitioning.txt | GE | Graph structure of the input node subgraph after the second split operation | - |
| 103 | ge_proto_xxxx_partition0_rank1_new_sub_graph1_SecondPartitioning.txt | GE | Graph structure of subgraph 1 after the second split operation | - |
| 104 | ge_proto_xxxx_partition0_rank2_new_sub_graph110_SecondPartitioning.txt | GE | Graph structure of subgraph 110 after the second split operation | - |
| 105 | ge_proto_xxxx_BeforeUnfoldSubgraphs.txt | GE | Graph before dynamic shape graph unfolding | - |
| 106 | ge_proto_xxxx_AfterUnfoldSubgraphs.txt | GE | Graph after dynamic shape graph unfolding | - |
| 107 | ge_proto_xxxx_RunCustomPass_BeforeAssignLogicStream{pass_name}.txt | GE | Graph before the user-defined stream assignment Pass processing | Trustlist |
| 108 | ge_proto_xxxx_RunCustomPass_AfterAssignLogicStream{pass_name}.txt | GE | Graph after the user-defined stream assignment Pass processing | Trustlist |
| 109 | ge_proto_xxxx_AfterAssignLogicalStreams.txt | GE | Graph after logical stream assignment is completed | - |
| 110 | ge_proto_xxxx_AfterAssignResource.txt | GE | Graph after stream assignment, memory allocation, and GenTask | - |
| 111 | ge_proto_xxxx_Build.txt | GE | Exit graph of model compilation | Trustlist |
| 112 | ge_proto_xxxx_PreRunAfterBuild.txt | GE | Same as the previous graph | - |
| 113 | ge_proto_xxxx_BeforeAttrsCompress.txt | GE | Graph before offline model compression | - |
| 114 | ge_proto_xxxx_AfterAttrsCompress.txt | GE | Graph after offline model compression | - |
| 115 | ge_proto_xxxx_BeforeAttrsDecompress.txt | GE | Graph before offline model decompression | - |
| 116 | ge_proto_xxxx_AfterAttrsDecompress.txt | GE | Graph after offline model decompression | - |
| 117 | ge_proto_xxxx_ComputeGraphBeforeLowering.txt | GE | Compute graph before lowering | Trustlist |
| 118 | ge_proto_xxxx_Before_MultiStream_LoweringFirstLastEventSync.txt | GE | Execution graph before inserting Events in multi-stream scenarios | - |
| 119 | ge_proto_xxxx_ExeGraphBeforeOptimize.txt | GE | Execution graph before execution graph optimization | Trustlist |
| 120 | ge_proto_xxxx_AfterZeroCopy.txt | GE | Execution graph after zero-copy optimization | - |
| 121 | ge_proto_xxxx_AfterCEM.txt | GE | Execution graph after CEM optimization | - |
| 122 | ge_proto_xxxx_AfterCopyFlowLaunch.txt | GE | Execution graph after in-path copy optimization | - |
| 123 | ge_proto_xxxx_TrustOutTensorAfter.txt | GE | Execution graph after TrustOutTensor optimization | - |
| 124 | ge_proto_xxxx_AfterAicpuFuseHostInputs.txt | GE | Graph after AicpuFuseHostInputs optimization | - |
| 125 | ge_proto_xxxx_ExecuteGraphAfterSplit.txt | GE | Final execution graph of dynamic shape | Trustlist |
