// dxil_spv_cfg_api.cpp - CFG extraction API for dxil-spirv
#include "dxil_spirv_c.h"
#include "dxil_converter.hpp"
#include "bc/module.hpp"
#include <stdint.h>
#include <stddef.h>

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
    
    /* The converter is actually a pointer to the internal Converter object */
    auto* impl = static_cast<dxil_spv::Converter*>(converter);
    if (!impl)
        return DXIL_SPV_ERROR_INVALID_ARGUMENT;
    
    /* Get the LLVM module from the bitcode parser */
    auto* module = impl->get_module();
    if (!module || !module->has_cfg_data())
        return DXIL_SPV_ERROR_NO_DATA;
    
    /* Return const pointers to the CFG data arrays stored in the module */
    *headers = module->get_cfg_headers().data();
    *merges = module->get_cfg_merges().data();
    *continues = module->get_cfg_continues().data();
    *hints = module->get_cfg_hints().data();
    *count = module->get_cfg_headers().size();
    
    return DXIL_SPV_SUCCESS;
}

} // extern "C"