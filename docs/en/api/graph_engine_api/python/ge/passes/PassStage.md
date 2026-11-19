# PassStage

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:49:22.231Z pushedAt=2026-10-16T10:53:07.594Z -->

Pass execution stage enumeration, used to specify the registration timing of a pass in the GE compilation process.

| Enum Value | Description |
| --- | --- |
| BEFORE_INFER_SHAPE | Before shape inference |
| AFTER_INFER_SHAPE | After shape inference |
| AFTER_BUILTIN_FUSION_PASS | After the built-in fusion pass |
| AFTER_ORIGIN_GRAPH_OPTIMIZE | After origin graph optimization |
