/* Switch-Dispatch SPIR-V Emitter for irreducible CFG
 *
 * When the MinimalStructurizer encounters irreducible control flow that
 * cannot be expressed with simple loop/selection constructs, this emitter
 * wraps the entire function body in a loop+switch state machine.
 *
 * Each original basic block becomes a case in the OpSwitch. Blocks store
 * their successor index in a "next_block" variable before branching to the
 * continue block. The switch then dispatches to the next block.
 *
 * Structured constructs (real loops, if/else diamonds) are emitted normally
 * INSIDE their switch case, preserving OpLoopMerge/OpSelectionMerge for
 * the driver optimizer.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "spirv_module.hpp"
#include "node.hpp"
#include "ir.hpp"
#include <stdint.h>

namespace dxil_spv
{

class SwitchDispatchEmitter
{
public:
        SwitchDispatchEmitter(SPIRVModule &module_);
        ~SwitchDispatchEmitter() = default;

        // Emit the entire function body using switch-dispatch.
        // Takes the entry CFGNode and the ordered list of all reachable blocks.
        void emit(CFGNode *entry, const Vector<CFGNode *> &blocks);

private:
        SPIRVModule &module;

        // Assign a sequential index to each block for the switch.
        UnorderedMap<CFGNode *, uint32_t> block_indices;

        // The "next_block" variable (Function-local uint32).
        uint32_t next_block_var_id = 0;
        uint32_t uint_type_id = 0;

        // Allocate indices and the variable.
        void assign_block_indices(const Vector<CFGNode *> &blocks);

        // Emit the switch-dispatch wrapper.
        void emit_dispatch_loop(CFGNode *entry, const Vector<CFGNode *> &blocks);

        // Emit a single block's operations inside its switch case.
        // For blocks that are loop/selection headers, emit them with proper
        // merge instructions so the driver can optimize them.
        void emit_block_as_case(CFGNode *node);

        // Emit a branch to the next block by storing the index and branching
        // to the continue block of the dispatch loop.
        void emit_dispatch_branch(CFGNode *target);
};

} // namespace dxil_spv
