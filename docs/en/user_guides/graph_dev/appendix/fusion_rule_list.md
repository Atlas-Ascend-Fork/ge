# Fusion Rules

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-19T12:28:26.163Z pushedAt=2026-10-21T02:37:32.453Z nl=2 -->

Disabling the following fusion rules may pose functional risks. For more fusion rules, see *[Graph Fusion and UB Fusion Rule Reference](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/maintenref/graphubfusionref/atlasrr_30_0003.html)*.

<!-- npu="950" id1 -->
The Ascend 950PR and 950DT products do not support UB fusion.
<!-- end id1 -->

  <!-- npu="950" id3 -->
- Graph fusion: graph fusion rules supported by the Ascend 950PR and 950DT products
  - AABiasaddConvFusion
  - AddRmsNormFusionGraphPass
  - ADepthwiseFusionPass
  - Conv2DQuantProcessFusionPass
  - DepthwiseDfFusionPass
  - DepthwiseDwMulFusionPass
  - EinsumPass
  - FixPipeAbilityProcessPass
  - InplaceAddRmsNormFusionPass
  - SoftmaxGradExtFusion
  - TfTagNoConstFoldingFusionPass
  - WeightQuantBatchMatmulV2TransposeFusionPass
  - ZConfusionSoftmaxGradFusionPass
  - ConstToAttrStridedSliceV2Fusion
  - DepthwiseToConv2dFusionPass
  - DWConv2DQuantProcessFusionPass
  - Globalavgpoolpass
  <!-- end id3 -->

- Graph fusion:

    <!-- npu="950" id2 -->
    Graph fusion rules supported by products other than the Ascend 950PR and 950DT products
    <!-- end id2 -->

  - AABiasaddConvFusion
  - AddNFusionPass
  - AddRmsNormFusionGraphPass
  - ADeformableConv2dPass
  - ADepthwiseFusionPass
  - ALSTMFusionPass
  - ApplyAddOutputPass
  - AReduceMeanFusionPass
  - AReduceSumFusionPass
  - AvgPool3DFusionPass
  - AvgPool3DGradFusionPass
  - AvgPoolGradFusionPass
  - AvgPoolQuantProcessFusionPass
  - BatchMatmulV2QuantProcessFusionPass
  - BatchNormBnInferFusionPass
  - BatchNormGradBnInferGradFusion
  - BatchNormGradInfGradFusion
  - BatchMatMulFusionPass
  - CastRemoveFusionPass
  - ConstToAttrPass
  - ConstToAttrReduceSumFusion
  - Conv2DQuantProcessFusionPass
  - CommonLSTMFusionPass
  - CommonSubexpressionEliminationPass: GE common subexpression elimination pass
  - ConstToAttrGatherV2Fusion
  - ConstToAttrResizeNearestNeighborGradFusion
  - ConstToAttrStridedSliceV2Fusion
  - Conv2DTDQuantProcessFusionPass
  - COPYPass
  - DeConvQuantProcessFusionPass
  - DepthwiseDfFusionPass
  - DepthwiseDwMulFusionPass
  - DepthwiseFusionPass
  - DepthwiseToConv2dFusionPass
  - DreluFusionPass
  - DWConv2DQuantProcessFusionPass
  - DynamicGRUV2GradFusionPass
  - DynamicRNNFusionPass
  - DynamicRNNGradAFusionPass
  - DynamicRNNGradAlignFusionPass
  - DynamicRNNGradDAlignFusionPass
  - DynamicRNNGradDFusionPass
  - DynamicRNNGradFusionPass
  - DynamicRNNInsertTransposePass
  - DynamicRNNSeqFusionPass
  - EinsumPass
  - FCQuantProcessFusionPass
  - FixPipeAbilityProcessPass
  - FlattenV2Pass
  - FusedBatchNormBertFusionPass
  - FusedBatchnormFusionPass
  - FusedBatchNormGradFusionPass
  - Globalavgpoolpass
  - GroupConv2DQuantProcessFusionPass
  - HostBNFusionPass
  - InplaceAddRmsNormFusionPass
  - MapIndexFusionPass
  - MatMul2MatMulV2FusionPass
  - MatmulV2QuantProcessFusionPass
  - MaxPoolWithArgmaxFusionPass
  - PackFusionPass
  - PassThroughFusionPass
  - PassThroughSecondFusionPass
  - PermuteFusionPass
  - PoolingQuantProcessFusionPass
  - PReluGradFusionPass
  - PriorBoxPass
  - ProposalFusionPass
  - RNNFusionPass
  - SingleBatchNormFusion
  - SpatialTransformerDPass
  - SPPPass
  - SoftmaxGradExtFusion
  - TbeAntiquantMaxpoolingFusionPass
  - TbeConvDequantS16FusionPass
  - TfMergeConv2DBackpropInputFusionPass
  - TfTagNoConstFoldingFusionPass
  - TransposedUpdateFusionPass
  - TransdataCastFusionPass
  - UnpackFusionPass
  - V100NotRequantFusionPass
  - V200NotRequantFusionPass
  - WeightQuantBatchMatmulV2TransposeFusionPass
  - YoloPass
  - YoloV2DetectionOutputPass
  - YoloV3DetectionOutputV2Pass
  - ZConcatExt2FusionPass
  - ZConcatDFusionPass
  - ZConfusionSoftmaxGradFusionPass
  - ZSplitVDFusionPass
  - ZSplitVFusionPass

- UB fusion:
  - ArgMaxWithFusionPass
  - clip\_by\_norm\_nodivsquaresum
  - DeformableOffsetsFusionPass
  - DepthwiseInsertTransDataFusionPass
  - HostShapeOptimizationPass: AI CPU pipeline interruption pass
  - NormalizeFusionPass
  - TbeConvRequantFusionPass
  - TbeConvCommonRules0FusionPass
  - TbeConvCommonRules2FusionPass
  - TfMergeSubFusionPass
