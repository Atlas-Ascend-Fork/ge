# DataType list attribute dispatch

## Behavior

A homogeneous list of DataType enum values must use SetListDataType. Ordinary integer, boolean, float and string lists retain their existing dispatch.

## Root cause and impact

DataType is an IntEnum. Checking integer lists first previously wrote a datatype-list attribute as LIST_INT, causing native attribute type mismatches.

Implementation: api/python/ge/ge/graph/_attr.py.

## Validation and limits

tests/ge/ut/ge/graph/pyge_tests/attr_list_datatype_dispatch_test.py loads actual Python source and substitutes only the native boundary. Local CPU regression: 5 passed in 0.06s. Real CANN/NPU integration, native ABI and full C++ suites have not been verified.

## Cache encapsulation

`_AttrValue.get_or_cache` owns typed cache access. The decorator forwards the getter, expected type and arguments without accessing protected members outside the class. The helper follows `__repr__`; getter failures propagate naturally and leave the cache unchanged. DataType list dispatch and existing setter/getter interfaces are preserved.
