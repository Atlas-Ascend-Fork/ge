# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:56:07.183Z pushedAt=2026-10-16T10:53:07.400Z -->

The **TensorHolder** class is used to represent tensor operations in **Eager Style** graph construction.

- **TensorHolder** objects cannot be created directly; they are created internally by **GraphBuilder** operations.
- **TensorHolder** cannot call setter methods after **GraphBuilder.build\_and\_reset\(\)**.
- All **TensorHolder** objects automatically resolve and maintain a strong reference to their **GraphBuilder** to ensure that the underlying C++ resources remain valid while the **TensorHolder** objects are still in use.
