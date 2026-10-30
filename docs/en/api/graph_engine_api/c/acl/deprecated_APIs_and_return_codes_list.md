# Deprecated APIs and Return Codes

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:08:32.533Z pushedAt=2026-10-29T04:03:00.679Z nl=2 -->

## API

- Single-operator call APIs

    The single-operator call APIs will be deprecated in future versions. Use the new APIs according to [Deprecated API Replacement Scheme](./deprecated_APIs_replacement_scheme.md).

- [aclGetTensorDescDim](aclGetTensorDescDim_deprecated.md) API

    This API will be deprecated in future versions. Use the [aclGetTensorDescDimV2](aclGetTensorDescDimV2.md) API instead.

- [aclSetTensorStorageShape](aclSetTensorStorageShape_deprecated.md) API

    This API will be deprecated in future versions. Use the [aclSetTensorShape](aclSetTensorShape.md) API instead.

- [aclSetTensorStorageFormat](aclSetTensorStorageFormat_deprecated.md) API

    This API will be deprecated in future versions. Use the [aclSetTensorFormat](aclSetTensorFormat.md) API instead.

## Return Code

- [ACL\_ERROR\_NONE](aclError.md) return code

    This return code will be deprecated in future versions. Use the [ACL\_SUCCESS](aclError.md) return code instead.

- [ACL\_ERROR\_NOT\_STATIC\_AIPP](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_GE\_AIPP\_NOT\_EXIST](aclError.md) return code instead.

- [ACL\_ERROR\_STREAM\_NOT\_SUBSCRIBE](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_RT\_STREAM\_NO\_CB\_REG](aclError.md) return code instead.

- [ACL\_ERROR\_THREAD\_NOT\_SUBSCRIBE](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_RT\_THREAD\_SUBSCRIBE](aclError.md) return code instead.

- [ACL\_ERROR\_WAIT\_CALLBACK\_TIMEOUT](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_RT\_REPORT\_TIMEOUT](aclError.md) return code instead.

- [ACL\_ERROR\_INVALID\_DEVICE](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_RT\_INVALID\_DEVICEID](aclError.md) return code instead.

- [ACL\_ERROR\_GROUP\_NOT\_SET](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_RT\_GROUP\_NOT\_SET](aclError.md) return code instead.

- [ACL\_ERROR\_GROUP\_NOT\_CREATE](aclError.md)

    This return code will be deprecated in future versions. Use the [ACL\_ERROR\_RT\_GROUP\_NOT\_CREATE](aclError.md) return code instead.
