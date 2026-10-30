# Crop/Padding Configuration

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T09:07:14.226Z pushedAt=2026-10-19T12:28:08.110Z -->

AIPP changes the image size in the order shown in the following figure, that is, Crop first and then Padding. Each operation can be performed only once.

An image whose original size is srcImageSizeW and srcImageSizeH is preprocessed into an image of the size dstImageSizeW and dstImageSizeH expected by the model.

**Figure 1** Changing the image size
![Diagram](../figures/change_image_size.png "Changing the image size")

> [!NOTE]
>
> In the figure, the solid-line box indicates the current image size, and the dashed-line box indicates the image size after the AIPP operation indicated by the arrow on the right is performed.

From the execution perspective, you need to specify in the configuration file the coordinates loadStartPosW and loadStartPosH of the upper-left point of the crop start position and the cropped image size crop\_size\_w and crop\_size\_h. In the padding phase, you need to specify the padding sizes around the cropped image, that is, left\_padding\_size, right\_padding\_size, top\_padding\_size, and bottom\_padding\_size. The final image size after the image size change must be equal to the width and height of the input image size in the model file, that is, **--input\_shape**.

For the YUV420SP\_U8 image type, the load\_start\_pos\_w and load\_start\_pos\_h parameters must be set to even numbers. The following is a configuration example:

```textproto
aipp_op {
    aipp_mode: static
    input_format: YUV420SP_U8

    src_image_size_w: 320
    src_image_size_h: 240

    crop: true
    load_start_pos_w: 10
    load_start_pos_h: 20
    crop_size_w: 50
    crop_size_h: 60


    padding: true
    left_padding_size: 20
    right_padding_size: 15
    top_padding_size: 20
    bottom_padding_size: 15
    padding_value: 0

}
```
