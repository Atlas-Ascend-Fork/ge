# DataType list attribute readback

## Behavior

LIST_DATA_TYPE values are converted from native enum numbers to Python DataType values. The returned list can be passed back to set_list_data_type. Ordinary int, float and bool getters are unchanged. Native arrays are released even if enum conversion fails.

## Validation and limits

The regression uses the actual attribute getter/setter implementation, actual enum classes and ctypes buffers. The graph C API is replaced at its boundary.

Test: `tests/ge/ut/ge/graph/pyge_tests/attr_datatype_readback_cpu_test.py`. CPU result: 5 passed in 0.06s. Real CANN/NPU integration and full C++ suites have not been verified.

## Cache encapsulation

`_AttrValue.get_or_cache` reads and updates the typed cache inside its owning class. The decorator only forwards the getter, expected type and arguments, avoiding access to protected cache members outside the class. Existing behavior for cache hits, type mismatches, `None`, exceptions and setter success or failure is preserved.

On 2026-10-10, the existing five CPU regressions passed on both Windows and Linux. Another 24 before/after comparisons on each platform preserved return values, getter calls and cache state. These checks replace only the graph C API boundary and do not validate real devices.

The cache helper follows `__repr__` in the project method order. Getter exceptions propagate directly; cache values and types are updated only after a successful call.
