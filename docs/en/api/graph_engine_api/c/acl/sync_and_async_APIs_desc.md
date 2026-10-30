# Synchronous and Asynchronous API Description

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:13:04.241Z pushedAt=2026-10-28T13:18:39.134Z nl=2 -->

CANN Runtime supports the following types of explicit synchronization. After such an API is called, the host thread blocks until the related tasks are complete.

- **Device synchronization: for example, aclrtSynchronizeDevice**

    Blocks the current host thread until all explicitly or implicitly created streams on the device complete all previously submitted tasks. Use this function as little as possible to avoid delaying host execution.

- **Stream synchronization: for example, aclrtSynchronizeStream**

    Blocks the current host thread until all submitted tasks in the specified stream are complete.

- **Event synchronization: for example, aclrtSynchronizeEvent**

    Blocks the current host thread until the specified event is complete. This is a finer-grained synchronization.

**For asynchronous APIs**, after the host thread calls an asynchronous API, it only means that the task has been submitted, not that the task has been executed successfully. Before the task is complete, the asynchronous API has already returned success to the host thread. You must explicitly call the synchronization APIs provided by Runtime to block the host thread and wait for task completion. Otherwise, unknown issues such as abnormal training or inference and device disconnection may occur.
