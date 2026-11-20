# Overview

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T03:56:36.840Z pushedAt=2026-10-16T10:53:07.626Z -->

A compile-time Pass context, which is the Python view of the C++ side **CustomPassContext**. It is injected by the engine into FusionBasePass.run\(graph, context\); when the `meet_requirements` or `replacement` of **PatternFusionPass** or **DecomposePass** declares this parameter, it is also injected into the corresponding callback to query or set the Pass name, error information, and compilation options. This object is valid only within the current callback stack.
