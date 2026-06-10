/* Structurizer Selection Interface
 *
 * Provides a compile-time and runtime switch between:
 *   - The original 7800-line CFGStructurizer (DEFAULT)
 *   - The new MinimalStructurizer (MINIMAL)
 *
 * This allows A/B testing without modifying the converter or SPIR-V emitter.
 * The MinimalStructurizer implements Approach 2: detect loops + diamonds,
 * fall back to switch-dispatch for irreducible CFG.
 *
 * Usage in dxil_converter.cpp or dxil_spirv_c.cpp:
 *
 *   auto structurizer = create_structurizer(entry, pool, module, StructurizerMode::MINIMAL);
 *   structurizer->run();
 *   module.emit_entry_point_function_body(*structurizer);
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "cfg_structurizer.hpp"
#include "minimal_structurizer.hpp"
#include <memory>

namespace dxil_spv
{

enum class StructurizerMode
{
        DEFAULT,   // Original CFGStructurizer (20-phase pipeline)
        MINIMAL,   // MinimalStructurizer (loops + diamonds + switch-dispatch fallback)
};

// Abstract interface that both structurizers implement.
// This allows the converter code to work with either one.
class StructurizerInterface
{
public:
        virtual ~StructurizerInterface() = default;
        virtual bool run() = 0;
        virtual void traverse(BlockEmissionInterface &iface) = 0;
        virtual CFGNode *get_entry_block() const = 0;

        // Compatibility methods called by SPIRVModule.
        virtual bool rewrite_rov_lock_region() = 0;
        virtual void rewrite_auto_group_shared_barrier() = 0;
        virtual void flatten_subgroup_shuffles() = 0;
        virtual void fixup_loop_header_undef_phis() = 0;
};

// Adapter for the original CFGStructurizer.
class OriginalStructurizerAdapter : public StructurizerInterface
{
public:
        OriginalStructurizerAdapter(CFGNode *entry, CFGNodePool &pool, SPIRVModule &module)
            : structurizer(entry, pool, module) {}

        bool run() override { return structurizer.run(); }
        void traverse(BlockEmissionInterface &iface) override { structurizer.traverse(iface); }
        CFGNode *get_entry_block() const override { return structurizer.get_entry_block(); }
        bool rewrite_rov_lock_region() override { return structurizer.rewrite_rov_lock_region(); }
        void rewrite_auto_group_shared_barrier() override { structurizer.rewrite_auto_group_shared_barrier(); }
        void flatten_subgroup_shuffles() override { structurizer.flatten_subgroup_shuffles(); }
        void fixup_loop_header_undef_phis() override { structurizer.fixup_loop_header_undef_phis(); }

        // Expose the underlying structurizer for methods that need its exact type.
        CFGStructurizer &get_structurizer() { return structurizer; }

private:
        CFGStructurizer structurizer;
};

// Adapter for the MinimalStructurizer.
class MinimalStructurizerAdapter : public StructurizerInterface
{
public:
        MinimalStructurizerAdapter(CFGNode *entry, CFGNodePool &pool, SPIRVModule &module)
            : structurizer(entry, pool, module) {}

        bool run() override { return structurizer.run(); }
        void traverse(BlockEmissionInterface &iface) override { structurizer.traverse(iface); }
        CFGNode *get_entry_block() const override { return structurizer.get_entry_block(); }
        bool rewrite_rov_lock_region() override { return structurizer.rewrite_rov_lock_region(); }
        void rewrite_auto_group_shared_barrier() override { structurizer.rewrite_auto_group_shared_barrier(); }
        void flatten_subgroup_shuffles() override { structurizer.flatten_subgroup_shuffles(); }
        void fixup_loop_header_undef_phis() override { structurizer.fixup_loop_header_undef_phis(); }

private:
        MinimalStructurizer structurizer;
};

// Factory function.
inline std::unique_ptr<StructurizerInterface> create_structurizer(
        CFGNode *entry, CFGNodePool &pool, SPIRVModule &module,
        StructurizerMode mode = StructurizerMode::DEFAULT)
{
        switch (mode)
        {
        case StructurizerMode::MINIMAL:
                return std::unique_ptr<StructurizerInterface>(
                        new MinimalStructurizerAdapter(entry, pool, module));

        case StructurizerMode::DEFAULT:
        default:
                return std::unique_ptr<StructurizerInterface>(
                        new OriginalStructurizerAdapter(entry, pool, module));
        }
}

} // namespace dxil_spv
