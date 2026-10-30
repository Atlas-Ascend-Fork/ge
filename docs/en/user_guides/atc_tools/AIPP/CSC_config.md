# Color Gamut Conversion Configuration

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-16T09:09:33.610Z pushedAt=2026-10-29T11:52:29.452Z nl=2 -->

AIPP provides a more convenient image format conversion method: color gamut conversion, which converts the input image format to the image format required by the model. Once the image formats before and after AIPP processing are confirmed, the parameter values related to color gamut conversion can be determined (the values of the **matrix\_r\*c\* configuration items are fixed and do not need to be adjusted**).

For example: convert JPEG image files (such as image files with the .jpg, .jpeg, .JPG, or .JPEG extension) to the RGB format; convert YUV data obtained after video decoding to the RGB format. According to different color video digitization standards, video formats can be further divided into the BT-601 standard-definition video format (defined in the SDTV standard) and the BT-709 high-definition video format (defined in the HDTV standard). Both video formats are further divided into NARROW and WIDE, where:

The NARROW value range is: ![Illustration](../figures/csc_config_narrow.png),
and the WIDE value range is: ![Illustration](../figures/csc_config_wide.png)

For details about how to determine the standard of the input data, see [How Do I Determine the Video Stream Format Standard When Using an AIPP Color Gamut Conversion Model](../FAQ/aipp_video_format_standard.md).

The conversion of YUV data to the RGB format can be regarded as the matrix multiplication shown in the following formula, where the conversion matrix consists of the parameters and offsets to be configured.

```text
# Convert YUV to BGR:
| B |   | matrix_r0c0 matrix_r0c1 matrix_r0c2 | | Y - input_bias_0 |
| G | = | matrix_r1c0 matrix_r1c1 matrix_r1c2 | | U - input_bias_1 | >> 8
| R |   | matrix_r2c0 matrix_r2c1 matrix_r2c2 | | V - input_bias_2 |
```

Before AIPP processing, for the images or videos input to the model (in various color encoding modes, such as YUV420SP\_U8 and RGB888\_U8), the color gamut conversion configurations for several typical scenarios are provided: JPEG, BT-601 NARROW, BT-601 WIDE, BT-709 NARROW, and BT-709 WIDE.

> [!NOTE]
>
>Data processed by DVPP does not affect the color gamut configuration. For example, if the data before DVPP processing is in the BT-709 format, the data remains in the BT-709 format after DVPP processing and before being input to AIPP.

## Color Gamut Conversion Overview

The supported color gamut conversion configurations are listed as follows:

- [YUV420SP\_U8 Convert to YUV444](#yuv420sp_u8-convert-to-yuv444)
- [YUV420SP\_U8 Convert to YVU444](#yuv420sp_u8-convert-to-yvu444)
- [YUV420SP\_U8 Converting to RGB](#yuv420sp_u8-converting-to-rgb)
- [YUV420SP\_U8 Converting to BGR](#yuv420sp_u8-converting-to-bgr)
- [YUV420SP\_U8 Converting to GRAY](#yuv400_u8-converting-to-gray)
- [YVU420SP\_U8 Convert to RGB](#yvu420sp_u8-convert-to-rgb)
- [YVU420SP\_U8 Convert to BGR](#yvu420sp_u8-convert-to-bgr)
- [RGB888\_U8 Converting to RGB](#rgb888_u8-converting-to-rgb)
- [RGB888\_U8 Convert to BGR](#rgb888_u8-convert-to-bgr)
- [RGB888\_U8 Converting to YUV444](#rgb888_u8-converting-to-yuv444)
- [RGB888\_U8 Converting to YVU444](#rgb888_u8-converting-to-yvu444)
- [RGB888\_U8 Converting to GRAY](#rgb888_u8-converting-to-gray)
- [BGR888\_U8 Converting to GRAY](#bgr888_u8-converting-to-gray)
- [BGR888\_U8 To RGB](#bgr888_u8-to-rgb)
- [BGR888\_U8 Convert to BGR](#bgr888_u8-convert-to-bgr)
- [XRGB8888\_U8 Converting to RGB](#xrgb8888_u8-converting-to-rgb)
- [XRGB8888\_U8 Convert to BGR](#xrgb8888_u8-convert-to-bgr)
- [XRGB8888\_U8 Converting to YUV444](#rgb888_u8-converting-to-yuv444)
- [XRGB8888\_U8 Convert to YVU444](#rgb888_u8-converting-to-yvu444)
- [XRGB8888\_U8 Converting to GRAY](#xrgb8888_u8-converting-to-gray)
- [XBGR8888\_U8 Convert to GRAY](#xbgr8888_u8-convert-to-gray)
- [RGBX8888\_U8 To GRAY](#rgbx8888_u8-to-gray)
- [BGRX8888\_U8 Convert to GRAY](#bgrx8888_u8-convert-to-gray)
- [YUV400\_U8 Converting to GRAY](#yuv400_u8-converting-to-gray)
- [RGB888\_U8 Convert to FP16 RGB](#rgb888_u8-convert-to-fp16-rgb)

## YUV420SP\_U8 Convert to YUV444

```textproto
aipp_op {
    aipp_mode: static
    input_format : YUV420SP_U8
    csc_switch : false
    rbuv_swap_switch : false
}
```

## YUV420SP\_U8 Convert to YVU444

```textproto
aipp_op {
    aipp_mode: static
    input_format : YUV420SP_U8
    csc_switch : false
    rbuv_swap_switch : true
}
```

## YUV420SP\_U8 Converting to RGB

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 359<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 454<br>   matrix_r2c2 : 0<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-601 NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 409<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -100<br>   matrix_r1c2 : -208<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 516<br>   matrix_r2c2 : 0<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-601 WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 359<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 454<br>   matrix_r2c2 : 0<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-709 NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 459<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -55<br>   matrix_r1c2 : -136<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 541<br>   matrix_r2c2 : 0<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-709 WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 403<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -48<br>   matrix_r1c2 : -120<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 475<br>   matrix_r2c2 : 0<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

## YUV420SP\_U8 Converting to BGR

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 454<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 359<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-601NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 516<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -100<br>   matrix_r1c2 : -208<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 409<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-601WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 454<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 359<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-709NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 541<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -55<br>   matrix_r1c2 : -136<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 459<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-709WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 475<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -48<br>   matrix_r1c2 : -120<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 403<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

## YUV420SP\_U8 Converting to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : YUV420SP_U8
    csc_switch : true
    rbuv_swap_switch : false
    matrix_r0c0 : 256
    matrix_r0c1 : 0
    matrix_r0c2 : 0
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    input_bias_0 : 0
    input_bias_1 : 0
    input_bias_2 : 0
}
```

## YVU420SP\_U8 Convert to RGB

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 359<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 454<br>   matrix_r2c2 : 0<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-601NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 409<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -100<br>   matrix_r1c2 : -208<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 516<br>   matrix_r2c2 : 0<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-601WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 359<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 454<br>   matrix_r2c2 : 0<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-709NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 459<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -55<br>   matrix_r1c2 : -136<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 541<br>   matrix_r2c2 : 0<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is a BT-709WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 0<br>   matrix_r0c2 : 403<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -48<br>   matrix_r1c2 : -120<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 475<br>   matrix_r2c2 : 0<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

## YVU420SP\_U8 Convert to BGR

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 454<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 359<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is BT-601NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 516<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -100<br>   matrix_r1c2 : -208<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 409<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is BT-601WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 454<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -88<br>   matrix_r1c2 : -183<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 359<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is BT-709NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 298<br>   matrix_r0c1 : 541<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 298<br>   matrix_r1c1 : -55<br>   matrix_r1c2 : -136<br>   matrix_r2c0 : 298<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 459<br>   input_bias_0 : 16<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

- The input data is BT-709WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : YUV420SP_U8<br>   csc_switch : true<br>   rbuv_swap_switch : true<br>   matrix_r0c0 : 256<br>   matrix_r0c1 : 475<br>   matrix_r0c2 : 0<br>   matrix_r1c0 : 256<br>   matrix_r1c1 : -48<br>   matrix_r1c2 : -120<br>   matrix_r2c0 : 256<br>   matrix_r2c1 : 0<br>   matrix_r2c2 : 403<br>   input_bias_0 : 0<br>   input_bias_1 : 128<br>   input_bias_2 : 128<br>   } |

## RGB888\_U8 Converting to RGB

```textproto
aipp_op {
    aipp_mode: static
    input_format : RGB888_U8
    csc_switch : false
    rbuv_swap_switch : false
}
```

## RGB888\_U8 Convert to BGR

```textproto
aipp_op {
    aipp_mode: static
    input_format : RGB888_U8
    csc_switch : false
    rbuv_swap_switch : true
}
```

## RGB888\_U8 Converting to YUV444

- The input data is a JPEG image.

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : -43<br>   matrix_r1c1 : -85<br>   matrix_r1c2 : 128<br>   matrix_r2c0 : 128<br>   matrix_r2c1 : -107<br>   matrix_r2c2 : -21<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-601NARROW video.

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 66<br>   matrix_r0c1 : 129<br>   matrix_r0c2 : 25<br>   matrix_r1c0 : -38<br>   matrix_r1c1 : -74<br>   matrix_r1c2 : 112<br>   matrix_r2c0 : 112<br>   matrix_r2c1 : -94<br>   matrix_r2c2 : -18<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-601WIDE video.

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : -43<br>   matrix_r1c1 : -85<br>   matrix_r1c2 : 128<br>   matrix_r2c0 : 128<br>   matrix_r2c1 : -107<br>   matrix_r2c2 : -21<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-709NARROW video.

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 47<br>   matrix_r0c1 : 157<br>   matrix_r0c2 : 16<br>   matrix_r1c0 : -26<br>   matrix_r1c1 : -87<br>   matrix_r1c2 : 112<br>   matrix_r2c0 : 112<br>   matrix_r2c1 : -102<br>   matrix_r2c2 : -10<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-709WIDE video.

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 55<br>   matrix_r0c1 : 183<br>   matrix_r0c2 : 19<br>   matrix_r1c0 : -29<br>   matrix_r1c1 : -99<br>   matrix_r1c2 : 128<br>   matrix_r2c0 : 128<br>   matrix_r2c1 : -116<br>   matrix_r2c2 : -12<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

## RGB888\_U8 Converting to YVU444

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : 128<br>   matrix_r1c1 : -107<br>   matrix_r1c2 : -21<br>   matrix_r2c0 : -43<br>   matrix_r2c1 : -85<br>   matrix_r2c2 : 128<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-601NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 66<br>   matrix_r0c1 : 129<br>   matrix_r0c2 : 25<br>   matrix_r1c0 : 112<br>   matrix_r1c1 : -94<br>   matrix_r1c2 : -18<br>   matrix_r2c0 : -38<br>   matrix_r2c1 : -74<br>   matrix_r2c2 : 112<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-601WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : 128<br>   matrix_r1c1 : -107<br>   matrix_r1c2 : -21<br>   matrix_r2c0 : -43<br>   matrix_r2c1 : -85<br>   matrix_r2c2 : 128<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-709NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 47<br>   matrix_r0c1 : 157<br>   matrix_r0c2 : 16<br>   matrix_r1c0 : 112<br>   matrix_r1c1 : -102<br>   matrix_r1c2 : -10<br>   matrix_r2c0 : -26<br>   matrix_r2c1 : -87<br>   matrix_r2c2 : 112<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is BT-709WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : RGB888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   matrix_r0c0 : 55<br>   matrix_r0c1 : 183<br>   matrix_r0c2 : 19<br>   matrix_r1c0 : 128<br>   matrix_r1c1 : -116<br>   matrix_r1c2 : -12<br>   matrix_r2c0 : -29<br>   matrix_r2c1 : -99<br>   matrix_r2c2 : 128<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

## RGB888\_U8 Converting to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : RGB888_U8
    csc_switch : true
    rbuv_swap_switch : false
    matrix_r0c0 : 76
    matrix_r0c1 : 150
    matrix_r0c2 : 30
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    output_bias_0 : 0
    output_bias_1 : 0
    output_bias_2 : 0
}
```

## BGR888\_U8 Converting to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : RGB888_U8
    csc_switch : true
    rbuv_swap_switch : true
    matrix_r0c0 : 76
    matrix_r0c1 : 150
    matrix_r0c2 : 30
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    output_bias_0 : 0
    output_bias_1 : 0
    output_bias_2 : 0
}
```

## BGR888\_U8 To RGB

```textproto
aipp_op {
    aipp_mode: static
    input_format : RGB888_U8
    csc_switch : false
    rbuv_swap_switch : true
}
```

## BGR888\_U8 Convert to BGR

```textproto
aipp_op {
    aipp_mode: static
    input_format : RGB888_U8
    csc_switch : false
    rbuv_swap_switch : false
}
```

## XRGB8888\_U8 Converting to RGB

```textproto
aipp_op {
    aipp_mode: static
    input_format : XRGB8888_U8
    csc_switch : false
    rbuv_swap_switch : false
    ax_swap_switch : true
}
```

## XRGB8888\_U8 Convert to BGR

```textproto
aipp_op {
aipp_mode: static
input_format : XRGB8888_U8
csc_switch : false
rbuv_swap_switch : true
ax_swap_switch : true
}
```

## XRGB8888\_U8 Converting to YUV444

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : -43<br>   matrix_r1c1 : -85<br>   matrix_r1c2 : 128<br>   matrix_r2c0 : 128<br>   matrix_r2c1 : -107<br>   matrix_r2c2 : -21<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-601NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 66<br>   matrix_r0c1 : 129<br>   matrix_r0c2 : 25<br>   matrix_r1c0 : -38<br>   matrix_r1c1 : -74<br>   matrix_r1c2 : 112<br>   matrix_r2c0 : 112<br>   matrix_r2c1 : -94<br>   matrix_r2c2 : -18<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-601WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : -43<br>   matrix_r1c1 : -85<br>   matrix_r1c2 : 128<br>   matrix_r2c0 : 128<br>   matrix_r2c1 : -107<br>   matrix_r2c2 : -21<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-709NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 47<br>   matrix_r0c1 : 157<br>   matrix_r0c2 : 16<br>   matrix_r1c0 : -26<br>   matrix_r1c1 : -87<br>   matrix_r1c2 : 112<br>   matrix_r2c0 : 112<br>   matrix_r2c1 : -102<br>   matrix_r2c2 : -10<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-709WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 55<br>   matrix_r0c1 : 183<br>   matrix_r0c2 : 19<br>   matrix_r1c0 : -29<br>   matrix_r1c1 : -99<br>   matrix_r1c2 : 128<br>   matrix_r2c0 : 128<br>   matrix_r2c1 : -116<br>   matrix_r2c2 : -12<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

## XRGB8888\_U8 Convert to YVU444

- The input data is a JPEG image

    | JPEG |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : 128<br>   matrix_r1c1 : -107<br>   matrix_r1c2 : -21<br>   matrix_r2c0 : -43<br>   matrix_r2c1 : -85<br>   matrix_r2c2 : 128<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-601NARROW video

    | BT-601NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 66<br>   matrix_r0c1 : 129<br>   matrix_r0c2 : 25<br>   matrix_r1c0 : 112<br>   matrix_r1c1 : -94<br>   matrix_r1c2 : -18<br>   matrix_r2c0 : -38<br>   matrix_r2c1 : -74<br>   matrix_r2c2 : 112<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-601WIDE video

    | BT-601WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 77<br>   matrix_r0c1 : 150<br>   matrix_r0c2 : 29<br>   matrix_r1c0 : 128<br>   matrix_r1c1 : -107<br>   matrix_r1c2 : -21<br>   matrix_r2c0 : -43<br>   matrix_r2c1 : -85<br>   matrix_r2c2 : 128<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-709NARROW video

    | BT-709NARROW |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 47<br>   matrix_r0c1 : 157<br>   matrix_r0c2 : 16<br>   matrix_r1c0 : 112<br>   matrix_r1c1 : -102<br>   matrix_r1c2 : -10<br>   matrix_r2c0 : -26<br>   matrix_r2c1 : -87<br>   matrix_r2c2 : 112<br>   output_bias_0 : 16<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

- The input data is a BT-709WIDE video

    | BT-709WIDE |
    | --- |
    | aipp_op {<br>   aipp_mode: static<br>   input_format : XRGB8888_U8<br>   csc_switch : true<br>   rbuv_swap_switch : false<br>   ax_swap_switch : true<br>   matrix_r0c0 : 55<br>   matrix_r0c1 : 183<br>   matrix_r0c2 : 19<br>   matrix_r1c0 : 128<br>   matrix_r1c1 : -116<br>   matrix_r1c2 : -12<br>   matrix_r2c0 : -29<br>   matrix_r2c1 : -99<br>   matrix_r2c2 : 128<br>   output_bias_0 : 0<br>   output_bias_1 : 128<br>   output_bias_2 : 128<br>   } |

## XRGB8888\_U8 Converting to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : XRGB8888_U8
    csc_switch : true
    rbuv_swap_switch : false
    ax_swap_switch : true
    matrix_r0c0 : 76
    matrix_r0c1 : 150
    matrix_r0c2 : 30
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    output_bias_0 : 0
    output_bias_1 : 0
    output_bias_2 : 0
}
```

## XBGR8888\_U8 Convert to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : XRGB8888_U8
    csc_switch : true
    rbuv_swap_switch : true
    ax_swap_switch : true
    matrix_r0c0 : 76
    matrix_r0c1 : 150
    matrix_r0c2 : 30
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    output_bias_0 : 0
    output_bias_1 : 0
    output_bias_2 : 0
}
```

## RGBX8888\_U8 To GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : XRGB8888_U8
    csc_switch : true
    rbuv_swap_switch : false
    ax_swap_switch : false
    matrix_r0c0 : 76
    matrix_r0c1 : 150
    matrix_r0c2 : 30
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    output_bias_0 : 0
    output_bias_1 : 0
    output_bias_2 : 0
}
```

## BGRX8888\_U8 Convert to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : XRGB8888_U8
    csc_switch : true
    rbuv_swap_switch : true
    ax_swap_switch : false
    matrix_r0c0 : 76
    matrix_r0c1 : 150
    matrix_r0c2 : 30
    matrix_r1c0 : 0
    matrix_r1c1 : 0
    matrix_r1c2 : 0
    matrix_r2c0 : 0
    matrix_r2c1 : 0
    matrix_r2c2 : 0
    output_bias_0 : 0
    output_bias_1 : 0
    output_bias_2 : 0
}
```

## YUV400\_U8 Converting to GRAY

```textproto
aipp_op {
    aipp_mode: static
    input_format : YUV400_U8
    csc_switch : false
}
```

## RGB888\_U8 Convert to FP16 RGB

```textproto
aipp_op {
    aipp_mode: static
    related_input_rank: 0
    input_format : RGB888_U8
    src_image_size_w : 640
    src_image_size_h : 640
    mean_chn_0 : 0
    mean_chn_1 : 0
    mean_chn_2 : 0
    var_reci_chn_0 : 1.0
    var_reci_chn_1 : 1.0
    var_reci_chn_2 : 1.0
}
```
