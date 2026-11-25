# Custom Fusion Pass

This directory provides development documentation and samples for GE custom fusion passes.

If this is your first development, it's recommended to read in the following order:

1. [Fusion Pattern Pass Mechanism](../../docs/en/design/features/fusion_pattern_pass.md): First understand pattern, matching, filtering, replacement and boundary rules.
2. [Python Fusion Pass Development Guide](python_fusion_pass_development_guide_en.md): Supports runtime integration and `@pattern` expression syntax.
3. [C++ Fusion Pass Development Guide](cpp_fusion_pass_development_guide_en.md): Suitable for product delivery after compiling into `.so`.

## Sample Directory

| Directory | Description |
|---|---|
| [pattern_base_pass](pattern_base_pass/README_en.md) | Recommended to reference first. Develop pattern-based fusion rules through `PatternFusionPass` or `DecomposePass` |
| [graph_base_pass](graph_base_pass/README_en.md) | Samples that directly modify graph through graph interfaces, suitable for scenarios requiring complete manual control of graph modification |
