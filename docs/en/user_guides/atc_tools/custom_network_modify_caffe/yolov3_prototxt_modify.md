# Modifying the YOLOv3 Network Model prototxt

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:15:29.691Z pushedAt=2026-10-28T07:49:38.955Z nl=2 -->

> [!NOTE]
>
> None of the code samples in this section can be directly copied into a network model. You need to adjust the corresponding parameters based on the network model you are using. For example, the parameters in bottom and top must correspond one-to-one with the bottom and top in the specific network model, and the order of the parameters corresponding to bottom and top cannot be changed.

1. Modify the attribute parameters of the upsample operator upsample\_param.

    Refer to *AI Framework Operator Support List*\> Supported Caffe Operator List. In the original operator prototxt, change scale:2 to scale:1  stride:2.

    The comparison is shown in [Figure 1](#fig1), where the left side is the original operator prototxt and the right side is the prototxt adapted to the AI processor.

    **Figure 1**  Comparison between the original operator and the modified operator 4<a id="fig1"></a>
    ![](../figures/original_vs_modified_op_4.png "Comparison between the original operator and the modified operator 4")

    For parameter explanation, see *AI Framework Operator Support List*\> Supported Caffe Operator List.

2. Add three Yolo operators.

    Because the Yolo operator plus the DetectionOutput operator constitute the complete post-processing logic of the feature detection network, as shown in the original operator prototxt, you need to add three Yolo operators before adding the YoloV3DetectionOutput operator.

    According to *AI Framework Operator Support List*\> Supported Caffe Operator List, this operator has one input and three outputs. Based on the preceding principle, the constructed Yolo operator code sample is as follows:

    - Code sample of operator 1:

        ```textproto
        layer {
         bottom: "layer82-conv"
         top: "yolo1_coords"
         top: "yolo1_obj"
         top: "yolo1_classes"
         name: "yolo1"
         type: "Yolo"
         yolo_param {
          boxes: 3
          coords: 4
          classes: 80
          yolo_version: "V3"
          softmax: true
          background: false
            }
        }
        ```

    - Code sample of operator 2:

        ```textproto
        layer {
         bottom: "layer94-conv"
         top: "yolo2_coords"
         top: "yolo2_obj"
         top: "yolo2_classes"
         name: "yolo2"
         type: "Yolo"
         yolo_param {
          boxes: 3
          coords: 4
          classes: 80
          yolo_version: "V3"
          softmax: true
          background: false
         }
        }
        ```

    - Code sample of operator 3:

        ```textproto
        layer {
         bottom: "layer106-conv"
         top: "yolo3_coords"
         top: "yolo3_obj"
         top: "yolo3_classes"
         name: "yolo3"
         type: "Yolo"
         yolo_param {
          boxes: 3
          coords: 4
          classes: 80
          yolo_version: "V3"
          softmax: true
          background: false
         }
        }
        ```

    For parameter explanation, see *AI Framework Operator Support List*\> Supported Caffe Operator List.

3. Add the YoloV3DetectionOutput operator to the last layer.

    For the YOLOv3 network, refer to [Extended Operator List](extended_operator_list.md) to add the post-processing operator layer YoloV3DetectionOutput at the end of the original prototxt file. See *AI Framework Operator Support List*\> Supported Caffe Operator List. The YoloV3DetectionOutput operator has ten inputs and two outputs. Based on this principle, the code sample of the constructed post-processing operator is as follows:

    ```textproto
    layer {
           name: "detection_out3"
           type: "YoloV3DetectionOutput"
           bottom: "yolo1_coords"
           bottom: "yolo2_coords"
           bottom: "yolo3_coords"
           bottom: "yolo1_obj"
           bottom: "yolo2_obj"
           bottom: "yolo3_obj"
           bottom: "yolo1_classes"
           bottom: "yolo2_classes"
           bottom: "yolo3_classes"
           bottom: "img_info"
           top: "box_out"
           top: "box_out_num"
           yolov3_detection_output_param {
                               boxes: 3
                               classes: 80
                               relative: true
                               obj_threshold: 0.5
                               score_threshold: 0.5
                               iou_threshold: 0.45
                               pre_nms_topn: 512
                               post_nms_topn: 1024
                               biases_high: 10
                               biases_high: 13
                               biases_high: 16
                               biases_high: 30
                               biases_high: 33
                               biases_high: 23
                               biases_mid: 30
                               biases_mid: 61
                               biases_mid: 62
                               biases_mid: 45
                               biases_mid: 59
                               biases_mid: 119
                               biases_low: 116
                               biases_low: 90
                               biases_low: 156
                               biases_low: 198
                               biases_low: 373
                               biases_low: 326
           }
    }
    ```

    For parameter explanation, see *AI Framework Operator Support List*\> Supported Caffe Operator List.

4. Add an input:

    Since the YoloV3DetectionOutput operator has img\_info inputs, this input is added when the model is input. The comparison is shown in [Figure 2](#fig2), where the left side is the original operator prototxt and the right side is the prototxt adapted to the AI processor.

    **Figure 2** Comparison 3 between the original operator and the modified operator<a id="fig2"></a>
    ![](../figures/original_vs_modified_op_3.png "Comparison 3 between the original operator and the modified operator")

    The code sample is as follows, where the parameter is \[batch,4\], and 4 indicates the four dimensions netH, netW, scaleH, and scaleW. Here, netH and netW are the HW of the network model input, and scaleH and scaleW are the HW of the original image.

    ```textproto
    input: "img_info"
    input_shape {
      dim: 1
      dim: 4
    }
    ```
