# GE\_ERRORNO

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:18:35.576Z pushedAt=2026-10-28T07:40:22.148Z nl=2 -->

- **GE\_ERRORNO**

    Error code and description registration macro. This macro provides the **SUCCESS** and **FAILED** error codes for users.

    GE\_ERRORNO\(0, 0, 0, 0, 0, SUCCESS, 0, "success"\);

    GE\_ERRORNO\(0b11, 0b11, 0b111, 0xFF, 0b11111, FAILED, 0xFFF, "failed"\);

- **GE\_ERRORNO\_DEFINE**

    Error code definition macro. This macro provides the END\_OF\_SEQUENCE error code for users.

    GE\_ERRORNO\_DEFINE\(0b01, 0b01, 0b000, 8, 0, END\_OF\_SEQUENCE, 7\);

- **GE\_ERRORNO\_EXTERNAL**

    Error code and error description information macro. **name** is of the uint32\_t type, and **desc** is of the **std::string** type.

    GE\_ERRORNO\_EXTERNAL\(ACL\_ERROR\_GE\_PARAM\_INVALID, "Parameter invalid."\);
