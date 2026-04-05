// include/dxil_cfg_reader.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dxil_cfg_block {
    uint32_t index;
    uint32_t header;
    uint32_t merge;
    uint32_t continue_block;
    uint32_t hint;
    uint32_t num_successors;
    uint32_t successors[8];
    uint32_t num_predecessors;
    uint32_t predecessors[8];
};

struct dxil_cfg_construct {
    uint32_t num_blocks;
    struct dxil_cfg_block* blocks;
    uint32_t entry_block;
    char error_message[256];
};

// Extract CFG from SPIR-V (your existing function)
struct dxil_cfg_construct* dxil_extract_cfg_from_spirv(const uint32_t* spirv_binary, size_t word_count);

// NEW: Extract CFG directly from DXIL module (using metadata)
struct dxil_cfg_construct* dxil_extract_cfg_from_module(void* llvm_module, const char* function_name);

void dxil_free_cfg(struct dxil_cfg_construct* cfg);

#ifdef __cplusplus
}
#endif