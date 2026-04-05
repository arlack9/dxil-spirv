// bc/cfg_metadata_parser.hpp
#pragma once

namespace LLVMBC {

// Forward declarations
struct ModuleParseContext;
struct BlockOrRecord;

// Parse !dx.controlFlowAnnotations metadata
void parse_control_flow_annotations(ModuleParseContext& ctx, const BlockOrRecord& entry);

} // namespace LLVMBC