# Tensor descriptor dimension range

## Behavior

Shape and TensorDesc reject dimensions outside signed int64 before constructing ctypes buffers. Construction, set_shape and set_origin_shape share this validation. Existing dynamic dimension markers and representable values remain unchanged.

## Validation and limits

The regression exercises actual Shape and TensorDesc code with actual ctypes buffers and enum classes. The graph C API is replaced at its boundary; no large tensor is allocated.

Test: `tests/ge/ut/ge/graph/pyge_tests/tensor_desc_dimension_range_cpu_test.py`. CPU result: 11 passed in 0.07s. Real CANN/NPU integration and full C++ suites have not been verified.
