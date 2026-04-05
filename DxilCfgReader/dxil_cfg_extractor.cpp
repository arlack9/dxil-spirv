// dxil_cfg_extractor.cpp
#include "dxil_cfg_reader.h"

// Use dxil-spirv's internal SPIRV-Cross headers
#include "third_party/SPIRV-Cross/spirv_cross.hpp"
#include "third_party/SPIRV-Cross/spirv_parser.hpp"

#include <cstring>
#include <cstdlib>
#include <vector>
#include <unordered_map>

using namespace spirv_cross;

extern "C" {

// Primary API - Extract CFG from SPIR-V binary
dxil_cfg_construct_t* dxil_extract_cfg_from_spirv(const uint32_t* spirv_binary, size_t word_count) {
    auto* cfg = new dxil_cfg_construct_t();
    memset(cfg, 0, sizeof(*cfg));
    
    // Validate input
    if (!spirv_binary || word_count == 0) {
        snprintf(cfg->error_message, sizeof(cfg->error_message), 
                 "Empty SPIR-V binary");
        return cfg;
    }
    
    // Parse SPIR-V binary
    std::vector<uint32_t> binary(spirv_binary, spirv_binary + word_count);
    Parser parser(binary);
    
    try {
        parser.parse();
    } catch (const std::exception& e) {
        snprintf(cfg->error_message, sizeof(cfg->error_message), 
                 "Failed to parse SPIR-V: %s", e.what());
        return cfg;
    }
    
    Compiler compiler(std::move(parser.get_parsed_ir()));
    const auto& ir = compiler.get_ir();
    
    // Map SPIRBlock IDs to sequential indices
    std::unordered_map<uint32_t, uint32_t> blockIdx;
    uint32_t idx = 0;
    
    for (const auto& id : ir.ids) {
        if (ir.ids[id].get_type() == TypeBlock) {
            blockIdx[id] = idx++;
        }
    }
    
    if (blockIdx.empty()) {
        snprintf(cfg->error_message, sizeof(cfg->error_message), 
                 "No basic blocks found in SPIR-V");
        return cfg;
    }
    
    // Get entry point block
    auto entry_points = compiler.get_entry_points_and_stages();
    if (!entry_points.empty()) {
        auto it = blockIdx.find(entry_points[0].entry_point);
        if (it != blockIdx.end()) {
            cfg->entry_block = it->second;
        }
    }
    
    // Allocate blocks array
    cfg->num_blocks = (uint32_t)blockIdx.size();
    cfg->blocks = (dxil_cfg_block_t*)calloc(cfg->num_blocks, sizeof(dxil_cfg_block_t));
    if (!cfg->blocks) {
        snprintf(cfg->error_message, sizeof(cfg->error_message), 
                 "Failed to allocate %u blocks", cfg->num_blocks);
        delete cfg;
        return nullptr;
    }
    
    // First pass: Fill block info and successors
    for (const auto& pair : blockIdx) {
        uint32_t block_id = pair.first;
        uint32_t i = pair.second;
        
        const auto& block = ir.get<SPIRBlock>(block_id);
        
        cfg->blocks[i].index = i;
        cfg->blocks[i].merge = block.merge;
        cfg->blocks[i].continue_block = block.continue_block;
        cfg->blocks[i].header = block.loop_dominator;
        cfg->blocks[i].hint = 0;  // SPIR-V doesn't have DXIL hints
        
        // Successors
        cfg->blocks[i].num_successors = (uint32_t)block.blocks.size();
        if (cfg->blocks[i].num_successors > 8) {
            cfg->blocks[i].num_successors = 8;
        }
        
        for (uint32_t s = 0; s < cfg->blocks[i].num_successors; s++) {
            auto it = blockIdx.find(block.blocks[s]);
            if (it != blockIdx.end()) {
                cfg->blocks[i].successors[s] = it->second;
            }
        }
    }
    
    // Second pass: Build predecessor lists
    for (uint32_t i = 0; i < cfg->num_blocks; i++) {
        cfg->blocks[i].num_predecessors = 0;
        for (uint32_t s = 0; s < cfg->blocks[i].num_successors; s++) {
            uint32_t succ = cfg->blocks[i].successors[s];
            if (succ < cfg->num_blocks) {
                uint32_t pred_idx = cfg->blocks[succ].num_predecessors;
                if (pred_idx < 8) {
                    cfg->blocks[succ].predecessors[pred_idx] = i;
                    cfg->blocks[succ].num_predecessors++;
                }
            }
        }
    }
    
    snprintf(cfg->error_message, sizeof(cfg->error_message),
             "CFG extracted: %u blocks, entry: %u, merges: %u",
             cfg->num_blocks, cfg->entry_block,
             [&cfg]() {
                 uint32_t count = 0;
                 for (uint32_t i = 0; i < cfg->num_blocks; i++)
                     if (cfg->blocks[i].merge != 0) count++;
                 return count;
             }());
    
    return cfg;
}

// Legacy API - kept for compatibility (returns error)
dxil_cfg_construct_t* dxil_extract_cfg(void* dxil_module) {
    auto* cfg = new dxil_cfg_construct_t();
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->error_message, sizeof(cfg->error_message),
             "This API is deprecated. Use dxil_extract_cfg_from_spirv() instead.");
    return cfg;
}

void dxil_free_cfg(dxil_cfg_construct_t* cfg) {
    if (cfg) {
        if (cfg->blocks) {
            free(cfg->blocks);
            cfg->blocks = nullptr;
        }
        delete cfg;
    }
}

} // extern "C"