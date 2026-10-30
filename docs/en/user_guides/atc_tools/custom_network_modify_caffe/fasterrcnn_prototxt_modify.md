# Modifying the Prototxt File of the FasterRCNN Network Model

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:09:52.143Z pushedAt=2026-10-28T02:20:45.178Z nl=2 -->

> [!NOTE]
>
> None of the code samples in this section can be directly copied into a network model. You need to adjust the corresponding parameters based on the network model you are using. For example, the parameters in bottom and top must correspond one-to-one to the bottom and top in the specific network model, and the order of the parameters corresponding to bottom and top cannot be changed.

The following uses the FasterRCNN Resnet34 network model as an example.

1. Modify the proposal operator.

    According to the *AI Framework Operator Support List*\> Caffe operator list, this operator has three inputs and two outputs. Based on the preceding principles, modify the original proposal operator, change type to the type in the caffe.proto file, and add the actual\_rois\_num output node. Add the corresponding attribute information by referring to the attribute definitions in the caffe.proto file. The modifications are shown in [Figure 1](#fig1), where the left side is the prototxt of the original operator and the right side is the prototxt adapted to the AI processor.

    **Figure 1**  Comparison 1 between the original operator and the modified operator<a id="fig1"></a>
    ![Illustration](../figures/original_vs_modified_op_1.png "Comparison 1 between the original operator and the modified operator")

    The code sample is as follows:

    ```textproto
    layer {
    name: "faster_rcnn_proposal"
    type: "Proposal"                  // Operator type


    bottom: "rpn_cls_prob_reshape"
    bottom: "rpn_bbox_pred"
    bottom: "im_info"
    top: "rois"
    top: "actual_rois_num"        // Added operator output
      proposal_param {

      feat_stride: 16
      base_size: 16
      min_size: 16
      pre_nms_topn: 3000
      post_nms_topn: 304
      iou_threshold: 0.7
      output_actual_rois_num: true
      }
    }
    ```

    For parameter description, see the *AI Framework Operator Support List*\> Caffe operator list.

2. Add the FSRDetectionOutput operator to the last layer to output the final detection result.

    For the FasterRCNN network, refer to [Extended Operator List](extended_operator_list.md) and add the post-processing operator layer FSRDetectionOutput to the end of the original prototxt file. See *AI Framework Operator Support List*\> Supported Caffe Operator List. The FSRDetectionOutput operator has five inputs and two outputs. The Type and attributes of this operator are defined as follows:

    The following is a code example:

    ```textproto
    layer {
      name: "FSRDetectionOutput_1"
      type: "FSRDetectionOutput"

      bottom: "rois"
      bottom: "bbox_pred"
      bottom: "cls_prob"
      bottom: "im_info"
      bottom: "actual_rois_num"
      top: "actual_bbox_num1"
      top: "box1"

      fsrdetectionoutput_param {

        num_classes:3
        score_threshold:0.0
        iou_threshold:0.7
        batch_rois:1
      }
    }
    ```

    For parameter description, see *AI Framework Operator Support List*\> Supported Caffe Operator List.
