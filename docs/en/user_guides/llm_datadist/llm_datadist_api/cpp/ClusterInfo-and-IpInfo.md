# ClusterInfo-and-IpInfo

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:49:45.190Z pushedAt=2026-11-17T03:39:12.087Z nl=2 -->

Describes cluster information and is used for link establishment and link teardown.

```cpp
struct ClusterInfo {
  uint64_t remote_cluster_id = 0U;     // cluster_id of the peer's LLM-DataDist
  int32_t remote_role_type = 0;        // role_type of the peer's LLM-DataDist. 0 indicates full, 1 indicates incremental
  std::vector<IpInfo> local_ip_infos;  // IP information of the local LLM-DataDist. For details, see the IpInfo structure below
  std::vector<IpInfo> remote_ip_infos; // IP information of the peer's LLM-DataDist. For details, see the IpInfo structure below
  uint8_t reserved[128];               // Reserved
};

struct IpInfo {
  AscendString ip;         // IP address
  uint16_t port = 0U;      // Port number
  uint8_t reserved[128];   // Reserved
};
```
