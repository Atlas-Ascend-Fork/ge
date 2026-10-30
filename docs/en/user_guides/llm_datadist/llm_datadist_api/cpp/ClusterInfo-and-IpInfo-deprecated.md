# ClusterInfo-and-IpInfo (deprecated)

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:11:05.858Z pushedAt=2026-10-29T03:25:50.714Z nl=2 -->

The data types ClusterInfo and IpInfo are defined in `${install_path}/latest/include/ge/llm_engine_types.h`, where `${install_path}` is the runtime installation path. The default path for the root user is `/usr/local/Ascend`. The details are as follows.

```cpp
struct ClusterInfo {
  uint64_t remote_cluster_id = 0U;     // cluster_id of the peer llm engine
  int32_t remote_role_type = 0;          // role_type of the peer llm engine. 0 indicates full, 1 indicates incremental
  std::vector<IpInfo> local_ip_infos;   // IP information of the local llm engine Device
  std::vector<IpInfo> remote_ip_infos; // IP information of the peer llm engine Device
};

struct IpInfo {
  uint32_t ip = 0U;   // uint32_t value converted from the little-endian IP address
  uint16_t port = 0U; // Port number. The client side does not need to specify it; the server side must specify the port (HCCL specifies a fixed port for users to fill in)
};
```
