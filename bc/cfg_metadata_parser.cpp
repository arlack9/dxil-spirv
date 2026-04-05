// bc/cfg_metadata_parser.cpp
#include "cfg_metadata_parser.hpp"
#include "module.hpp"
#include "metadata.hpp"

namespace LLVMBC {

// Helper to extract integer from metadata operand
static uint32_t extract_uint32_from_metadata(MDOperand* operand) {
    if (!operand) return 0;
    
    if (operand->get_metadata_kind() == MetadataKind::Constant) {
        auto* constant = static_cast<ConstantAsMetadata*>(operand);
        if (auto* int_const = llvm_cast<ConstantInt>(constant->getValue())) {
            return uint32_t(int_const->getLimitedValue());
        }
    }
    return 0;
}

// Parse !dx.controlFlowAnnotations metadata
void parse_control_flow_annotations(ModuleParseContext& ctx, const BlockOrRecord& entry) {
    // Record format: [name_length, char1, char2, ..., num_operands, operand1, ...]
    if (entry.record.size() < 2) return;
    
    uint64_t name_length = entry.record[0];
    if (entry.record.size() < 1 + name_length) return;
    
    std::string name;
    for (unsigned i = 0; i < name_length; i++) {
        name.push_back(char(entry.record[1 + i]));
    }
    
    // Only process control flow annotations
    if (name != "dx.controlFlowAnnotations") return;
    
    if (entry.record.size() <= 1 + name_length) return;
    uint64_t num_operands = entry.record[1 + name_length];
    
    // Initialize vectors to store CFG data
    std::vector<uint32_t> headers, merges, continues, hints;
    
    // Parse each operand (each is a function's CFG data)
    for (unsigned i = 0; i < num_operands; i++) {
        size_t idx = 1 + name_length + 1 + i;
        if (idx >= entry.record.size()) break;
        
        uint64_t node_index = entry.record[idx];
        auto it = ctx.metadata.find(node_index);
        if (it == ctx.metadata.end()) continue;
        
        MDOperand* operand = it->second;
        if (operand->get_metadata_kind() != MetadataKind::Node) continue;
        
        MDNode* func_node = static_cast<MDNode*>(operand);
        
        // Parse per-block hints
        for (unsigned j = 0; j < func_node->getNumOperands(); j++) {
            MDOperand* block_op = &func_node->getOperand(j);
            if (block_op->get_metadata_kind() != MetadataKind::Node) continue;
            
            MDNode* block_node = static_cast<MDNode*>(block_op);
            if (block_node->getNumOperands() >= 4) {
                uint32_t hint = extract_uint32_from_metadata(&block_node->getOperand(0));
                uint32_t header = extract_uint32_from_metadata(&block_node->getOperand(1));
                uint32_t merge = extract_uint32_from_metadata(&block_node->getOperand(2));
                uint32_t continue_block = extract_uint32_from_metadata(&block_node->getOperand(3));
                
                // Store CFG data
                headers.push_back(header);
                merges.push_back(merge);
                continues.push_back(continue_block);
                hints.push_back(hint);
            }
        }
    }
    
    // Store all CFG data in the module if any was found
    if (!headers.empty()) {
        ctx.module->set_cfg_data(
            std::move(headers),
            std::move(merges),
            std::move(continues),
            std::move(hints)
        );
    }
}

} // namespace LLVMBC