# Extended Operator List

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:08:27.712Z pushedAt=2026-10-27T14:40:41.967Z nl=2 -->

## Process Operators

| Operator Type | Description |
| --- | --- |
| [Proposal](extended_operator_rules.md#proposal) | Used in Faster R-CNN. Corrects anchors based on the foreground in rpn_cls_prob and the bounding box regression in rpn_bbox_pred to obtain accurate proposals. |
| [ROIPooling](extended_operator_rules.md#roipooling) | Used in Faster R-CNN. Performs pooling on ROI (Region of interest), mainly for object detection tasks. |
| [PSROIPooling](extended_operator_rules.md#psroipooling) | Used in R-FCN. Performs position-sensitive ROI pooling, mainly for object detection tasks. |
| [Reverse](extended_operator_rules.md#reverse) | Reverses the specified axis of the input tensor. |
| [Upsample](extended_operator_rules.md#upsample) | Upsampling using the pooling mask.<br>Used in the YOLO network. |
| [Normalize](extended_operator_rules.md#normalize) | Normalizes the elements in the channel dimension of the SSD network using L2 normalization. |
| [Reorg](extended_operator_rules.md#reorg) | Reorg layer in Darknet. Transfers channel data to the plane, or performs the reverse operation.<br>Corresponds to the PassThrough operator in the operator specification. |
| [ROIAlign](extended_operator_rules.md#roialign) | Obtains the feature matrix of ROI (Region of interest) from the feature map. |
| [ShuffleChannel](extended_operator_rules.md#shufflechannel) | Divides the channel dimension into [group, channel/Group], and then transposes the data in the channel dimension to [channel/Group, group]. |
| [Yolo](extended_operator_rules.md#yolo) | The Yolo/Detection/Region operators all need to be replaced with the Yolo operator, which generates the coordinate information, confidence information, and class probability of detection boxes from the feature map of the convolutional network output. |
| [PriorBox](extended_operator_rules.md#priorbox) | Used in the SSD network. Generates prior boxes based on input parameters. |
| [SpatialTransformer](extended_operator_rules.md#spatialtransformer) | Used for affine transformation. |

## Post-processing Operators

| Operator Type | Description |
| --- | --- |
| YoloV3DetectionOutput | This operator is used in the post-processing of YOLOv3 to generate the coordinates of detection boxes, confidence information, and class probabilities from the feature map of the convolutional network output. |
| YoloV2DetectionOutput | This operator is used in the post-processing of YOLOv2 to generate the coordinates of detection boxes, confidence information, and class probabilities from the feature map of the convolutional network output. |
| SSDDetectionOutput | This operator is used in the post-processing of the SSD network to integrate the results of prior boxes, prior box offsets, and scores, and finally output the target detection boxes that meet the conditions, as well as the labels and scores of the targets. |
| FSRDetectionOutput | This operator is used in the post-processing of the Faster R-CNN network to classify the results and output the final number of bboxes, coordinates, class probabilities, and class indexes for each class. |
