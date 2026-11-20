# Exception Handling

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:39:34.265Z pushedAt=2026-10-28T07:42:19.743Z nl=2 -->

## event\_sync\_timeout

Timeout for event synchronization wait during graph execution. A synchronization failure is reported when the configured time is exceeded. Unit: ms

Default value: -1, indicating no wait time. No error is reported when a synchronization failure occurs.

**Configuration example:**

```c++
{"event_sync_timeout", "-1"};
```

**Required/Optional**: Optional

**Effective level**: Global/session

## stream\_sync\_timeout

Stream synchronization wait timeout during graph execution. A synchronization failure is reported when the configured time is exceeded. Unit: ms

The default value is -1, which means no wait time and no error is reported when a synchronization failure occurs.

**Configuration example:**

```c++
{"stream_sync_timeout", "-1"};
```

**Required/Optional**: Optional

**Effective level**: global/session
