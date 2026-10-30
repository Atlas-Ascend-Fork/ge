# Typical Scenario Examples

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-16T09:11:39.794Z pushedAt=2026-10-20T02:26:29.797Z -->

## YUV400\_U8 to GRAY Format

- **Scenario Description**:

    The AIPP input image format is YUV400\_U8, the output image format is GRAY, the input image dimensions are 224\*224, and the valid data region starts from the \(0, 0\) pixel at the upper-left corner. The original network model's C=1, and both H and W are 220.

- **This scenario involves the following AIPP configurations**:
  - Enable the crop parameter for the cropping function;
  - The horizontal and vertical coordinates load\_start\_pos\_h and load\_start\_pos\_w of the crop start position are 0;
  - No configuration required for the crop\_size\_w and crop\_size\_h parameters. In this case, the width and height of the crop size (crop\_size\[W|H\]) are taken from the width and height in the --input\_shape parameter during model conversion, and the data of the 220\*220 region is selected starting from the \(0, 0\) point;
  - No configuration required for the color gamut conversion switch csc\_switch. Moreover, for the same original network model, if the AIPP input is a YUV420SP\_U8 image, the same set of AIPP configurations can be used, that is, only the Y channel data is taken.

- **AIPP configuration file example below:**

    ```textproto
    aipp_op{
        aipp_mode: static
        csc_switch: false
        crop: true
        input_format: YUV400_U8
        load_start_pos_h: 0
        load_start_pos_w: 0
        src_image_size_w: 224
        src_image_size_h: 224
        # The normalization coefficients need to be configured based on the actual requirements of the user model. The common values listed below are only examples.
        mean_chn_0: 128
        min_chn_0: 0.0
        var_reci_chn_0: 0.00390625
    }
    ```

## YUV420SP\_U8 Convert to BGR Format

- **Scenario Description:**

    The AIPP input image format is YUV420SP\_U8 (NV12), the output image format is BGR, and the input image dimensions are 256\*256. The original network model's C = 3, and H and W are the same as the AIPP input image dimensions.

- **This scenario involves the following AIPP configurations:**
  - No configuration required for the crop parameter of the cropping function.
  - The Color Gamut Conversion switch csc\_switch and the corresponding CSC matrix parameters need to be configured.

- **AIPP configuration file example below:**

    ```textproto
    aipp_op {
        aipp_mode: static
        input_format: YUV420SP_U8
        csc_switch: true
        # If the input is a YVU420SP_U8 (NV21) image, set the rbuv_swap_switch parameter to true.
        rbuv_swap_switch: false
        related_input_rank: 0
        src_image_size_w: 256
        src_image_size_h: 256
        crop: false
        matrix_r0c0: 298
        matrix_r0c1: 516
        matrix_r0c2: 0
        matrix_r1c0: 298
        matrix_r1c1: -100
        matrix_r1c2: -208
        matrix_r2c0: 298
        matrix_r2c1: 0
        matrix_r2c2: 409
        input_bias_0: 16
        input_bias_1: 128
        input_bias_2: 128
        # The normalization coefficients must be configured based on the actual requirements of the user model. The common values listed below are only examples.
        # The normalization coefficients are applied to the channels after color gamut conversion and channel swap.
        mean_chn_0: 104
        mean_chn_1: 117
        mean_chn_2: 123
        min_chn_0: 0.0
        min_chn_1: 0.0
        min_chn_2: 0.0
        var_reci_chn_0: 1.0
        var_reci_chn_1: 1.0
        var_reci_chn_2: 1.0
    }
    ```

## RGB888\_U8 to RGB (or BGR) Format

- **Scenario Description:**

    The AIPP input image format is RGB888\_U8, the output image format is RGB, the input image dimensions are 250\*250, and the valid data region starts from the pixel at the top-left corner \(0, 0\). The original network model has C=3, and both H and W are 240.

- **This scenario involves the following AIPP configurations:**
  - Enable the crop parameter for the cropping function.
  - The horizontal and vertical coordinates of the cropping start position load\_start\_pos\_h and load\_start\_pos\_w are 0.
  - No configuration is required for the crop\_size\_w and crop\_size\_h parameters. In this case, the width and height of the cropping size (crop\_size\[W|H\]) are taken from the width and height in the --input\_shape parameter during model conversion, and data in the 240\*240 region is selected starting from the \(0, 0\) point.
  - No configuration is required for the channel swap switch parameter rbuv\_swap\_switch, the color gamut conversion switch parameter csc\_switch, and the CSC matrix parameters.

- **AIPP configuration file example below:**

    ```textproto
    aipp_op {
        aipp_mode: static
        input_format: RGB888_U8
        csc_switch: false
        related_input_rank: 0
        src_image_size_w: 250
        src_image_size_h: 250
        crop: true
        load_start_pos_w: 0
        load_start_pos_h: 0
        # If the original model requires the BGR format, set the rbuv_swap_switch parameter to true.
        rbuv_swap_switch: false
        # The normalization coefficient must be configured based on the actual requirements of the user's model. The default value is used here, which means the pixel values remain unchanged.
        # If a normalization coefficient is configured, it is applied to the channels after the channel swap.
    }
    ```
