# Fusion Rules List

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:30:36.427Z pushedAt=2026-10-28T06:36:37.482Z nl=2 -->

Disabling the following fusion rules may pose functional risks. For more fusion rules, see *Graph Fusion and UB Fusion Rules Reference*.

<!-- npu="950" id1 -->
The Ascend 950PR&950DT products do not support UB fusion.
<!-- end id1 -->

<!-- npu="950" id2 -->
- Graph fusion: graph fusion rules supported by the Ascend 950PR&950DT products
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
<!-- end id2 -->

- Graph fusion:

    <!-- npu="950" id3 -->
    Graph fusion rules supported by non-Ascend 950PR&950DT products
    <!-- end id3 -->

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
  - HostShapeOptimizationPass: AI CPU pipeline-breaking pass
  - NormalizeFusionPass
  - TbeConvRequantFusionPass
  - TbeConvCommonRules0FusionPass
  - TbeConvCommonRules2FusionPass
  - TfMergeSubFusionPass
