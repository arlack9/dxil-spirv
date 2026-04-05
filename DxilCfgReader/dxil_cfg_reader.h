// dxil_cfg_reader.h - Minimal CFG Reader API
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shader kinds (from dxil-spirv)
typedef enum dxil_shader_kind_t {
    DXIL_SHADER_PIXEL = 0,
    DXIL_SHADER_VERTEX,
    DXIL_SHADER_GEOMETRY,
    DXIL_SHADER_HULL,
    DXIL_SHADER_DOMAIN,
    DXIL_SHADER_COMPUTE,
    DXIL_SHADER_LIBRARY,
    DXIL_SHADER_INVALID
} dxil_shader_kind_t;

// CFG Block information
typedef struct dxil_cfg_block_t {
    uint32_t index;              // Block index in function
    uint32_t header;             // Header block (0 = none)
    uint32_t merge;              // Merge block (0 = none)
    uint32_t continue_block;     // Continue block for loops (0 = none)
    uint32_t hint;               // ControlFlowHint value
    uint32_t num_successors;
    uint32_t successors[8];      // Maximum 8 successors
    uint32_t num_predecessors;
    uint32_t predecessors[8];
} dxil_cfg_block_t;

// CFG Construct
typedef struct dxil_cfg_construct_t {
    uint32_t num_blocks;
    dxil_cfg_block_t* blocks;
    uint32_t entry_block;
    char error_message[256];
} dxil_cfg_construct_t;

// API Functions
dxil_cfg_construct_t* dxil_extract_cfg(void* dxil_module);  // Takes DxilModule* (deprecated)
dxil_cfg_construct_t* dxil_extract_cfg_from_spirv(const uint32_t* spirv_binary, size_t word_count);
void dxil_free_cfg(dxil_cfg_construct_t* cfg);

#ifdef __cplusplus
}
#endif