# get\_user\_data (UDF)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:01:07.323Z pushedAt=2026-10-29T03:14:31.344Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains user-defined data.

## Prototype

```python
get_user_data(self, size: int , offset: int = 0) -> bytearray
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Input | Length of the user data. Value range: [0, 64]. |
| offset | Input | Offset of the user data, which must comply with the following constraints.<br>[0, 64), size+offset<=64 |

## Return Value

Returns user-defined data of the bytearray type.

## Exception Handling

None

## Constraints

When offset is not passed, the default value is 0. The returned type is bytearray, and users need to parse it in reverse according to the defined structure.

- The string type is parsed using byte\_array.decode\("utf-8"\).
- The int type can be parsed using int.from\_bytes\(byte\_array, byteorder='big'\), with byteorder set according to the environment.
- The float type can be parsed using struct.unpack\('f', byte\_array\)\[0\].
