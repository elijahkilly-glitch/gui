#pragma once
/*
 * NullKDriver/shared.h  —  STUB
 * Replace with your real kernel driver shared header.
 * The driver comms will fall back to ReadProcessMemory automatically.
 */
#include <Windows.h>
#include <cstdint>

#define REQUEST_MAGIC 0xDEADBEEFCAFEBABEULL

enum DriverCommand : uint32_t {
    CMD_PING        = 0,
    CMD_READ        = 1,
    CMD_WRITE       = 2,
    CMD_MODULE_BASE = 3,
};

struct REQUEST_DATA {
    uint64_t  magic       = 0;
    uint32_t  command     = 0;
    uint32_t  _pad        = 0;
    uint64_t  pid         = 0;
    uint64_t  address     = 0;
    uint64_t  buffer      = 0;
    uint64_t  size        = 0;
    uint64_t  result      = 0;
    wchar_t   module_name[64] = {};
};
