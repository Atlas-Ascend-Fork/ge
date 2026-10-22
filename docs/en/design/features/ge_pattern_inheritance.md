# Inherited expression-style fusion patterns

## Behavior

Adding a decorated pattern in a derived PatternFusionPass preserves visible base patterns. Method discovery follows the Python MRO and shadows overridden names. Explicit legacy patterns() style selection remains unchanged.

## Root cause and impact

The regenerated child adapter previously inspected only the child class dictionary, replacing the inherited adapter and silently losing base patterns. Native graph matching and NPU optimization have not been tested.

Implementation: api/python/ge/ge/passes/pattern.py.

## Validation and limits

tests/ge/ut/ge/graph/pyge_tests/pattern_inheritance_cpu_test.py loads actual Python source and substitutes only the native boundary. Local CPU regression: 6 passed in 0.08s. Real CANN/NPU integration, native ABI and full C++ suites have not been verified.
