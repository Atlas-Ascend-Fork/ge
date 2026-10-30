# aclAippInputFormat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:28:51.914Z pushedAt=2026-10-28T13:13:01.382Z nl=2 -->

Static AIPP and dynamic AIPP support different image formats, as detailed below:

```c
typedef enum
{
    ACL_YUV420SP_U8 = 1,    // YUV420SP_U8. Supported by both static AIPP and dynamic AIPP.
    ACL_XRGB8888_U8=2,      // XRGB8888_U8. Supported by both static AIPP and dynamic AIPP.
    ACL_RGB888_U8=3,        // RGB888_U8. Supported by both static AIPP and dynamic AIPP.
    ACL_YUV400_U8=4,        // YUV400_U8. Supported by both static AIPP and dynamic AIPP.
    ACL_NC1HWC0DI_FP16 = 5, // Not supported yet.
    ACL_NC1HWC0DI_S8 = 6,   // Not supported yet.
    ACL_ARGB8888_U8 = 7,    // Not supported yet.
    ACL_YUYV_U8 = 8,        // Not supported yet.
    ACL_YUV422SP_U8 = 9,    // Not supported yet.
    ACL_AYUV444_U8 = 10,    // Not supported yet.
    ACL_RAW10 = 11,         // Not supported yet.
    ACL_RAW12 = 12,         // Not supported yet.
    ACL_RAW16 = 13,         // Not supported yet.
    ACL_RAW24 = 14,         // Not supported yet.
    ACL_AIPP_RESERVED = 0xffff,
} aclAippInputFormat;
```
