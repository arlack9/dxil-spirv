// dxil_spv_cfg_api.cpp - CFG extraction API for dxil-spirv
#include "dxil_spirv_c.h"

extern "C" {

DXIL_SPV_PUBLIC_API dxil_spv_result dxil_spv_converter_get_cfg(
    dxil_spv_converter converter,
    const uint32_t** headers,
    const uint32_t** merges,
    const uint32_t** continues,
    const uint32_t** hints,
    size_t* count)
{
    if (!converter || !headers || !merges || !continues || !hints || !count)
        return DXIL_SPV_ERROR_INVALID_ARGUMENT;
      auto* conv_struct = static_cast<struct dxil_spv_converter_s*>(converter);
    if (!conv_struct)
        return DXIL_SPV_ERROR_INVALID_ARGUMENT;
    
    /* Get the LLVM module from the bitcode parser */
    auto* module = conv_struct->bc_parser.get_module();
    if (!module)
        return DXIL_SPV_ERROR_NOT_FOUND;
    
    if (!module->has_cfg_data())
        return DXIL_SPV_ERROR_NOT_FOUND;
    
    *headers = module->get_cfg_headers().data();
    *merges = module->get_cfg_merges().data();
    *continues = module->get_cfg_continues().data();
    *hints = module->get_cfg_hints().data();
    *count = module->get_cfg_headers().size();
    
    return DXIL_SPV_SUCCESS;
}

} // extern "C"