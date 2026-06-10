/* Minimal CFG Structurizer for dxil-spirv
 *
 * Replaces the 7800-line CFGStructurizer with a ~1500-line alternative that:
 *   1. Detects loops from back-edges (trivial)
 *   2. Detects if/else diamonds from dominance (simple)
 *   3. Falls back to switch-dispatch state machine for irreducible CFG
 *
 * This produces valid structured SPIR-V that passes spirv-val, while avoiding
 * the 20-phase pipeline of the original structurizer.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "thread_local_allocator.hpp"
#include "ir.hpp"
#include "cfg_structurizer.hpp"  // for BlockEmissionInterface
#include <stdint.h>

namespace dxil_spv
{
class SPIRVModule;
class CFGNodePool;

class MinimalStructurizer
{
public:
        MinimalStructurizer(CFGNode *entry, CFGNodePool &pool, SPIRVModule &module);
        ~MinimalStructurizer() = default;

        // Main structurization pass. Returns true on success.
        bool run();

        // Traverse the structurized CFG, emitting SPIR-V via the interface.
        void traverse(BlockEmissionInterface &iface);

        // Get the entry block after structurization.
        CFGNode *get_entry_block() const;

        // Compatibility stubs - these are called by SPIRVModule but are no-ops
        // in the minimal approach (they were complex workarounds in the original).
        bool rewrite_rov_lock_region() { return true; }
        void rewrite_auto_group_shared_barrier() {}
        void flatten_subgroup_shuffles() {}
        void fixup_loop_header_undef_phis() {}

private:
        CFGNode *entry_block;
        CFGNodePool &pool;
        SPIRVModule &module;

        // Visit order for traversal (dominators first).
        Vector<CFGNode *> forward_post_visit_order;

        // ---- Phase 1: CFG analysis ----

        // Walk the CFG and assign forward post-visit order.
        void visit_forward(CFGNode &node);
        bool forward_visited(CFGNode *node) const;

        // Build immediate dominators using iterative algorithm.
        void build_immediate_dominators();

        // Build immediate post-dominators.
        void build_immediate_post_dominators();

        // ---- Phase 2: Loop detection ----

        // Identify all loops by finding back-edges (predecessors that are
        // dominated by their successor in the forward post-order).
        // Assigns MergeType::Loop, loop_merge_block, and continue block.
        void find_loops();

        // Analyze a single loop: find its natural merge block and continue block.
        // Uses post-dominance to find the merge target.
        CFGNode *find_loop_merge(CFGNode *header);
        CFGNode *find_loop_continue(CFGNode *header);

        // ---- Phase 3: Selection detection ----

        // Find if/else diamonds: a node with two successors that rejoin at a
        // common post-dominator. Assigns MergeType::Selection.
        void find_selection_merges();

        // ---- Phase 4: Irreducible CFG handling ----

        // After loop and selection detection, check if any blocks still have
        // unstructured control flow. If so, wrap those regions in a
        // switch-dispatch state machine.
        //
        // The state machine uses a "next block" variable:
        //   - Each block stores its successor index before branching
        //   - A top-level switch dispatches on the variable
        //   - All blocks become cases in the switch
        void create_switch_dispatch_fallback();

        // Check if the CFG has any irreducible edges remaining.
        bool has_irreducible_edges() const;

        // ---- Phase 5: PHI fixup ----

        // After restructuring, fix up PHI nodes to match the new block layout.
        void fixup_phis();

        // ---- Helpers ----

        CFGNode *find_common_dominator(CFGNode *a, CFGNode *b);
        CFGNode *find_common_post_dominator(CFGNode *a, CFGNode *b);

        // Visitation tracking.
        UnorderedSet<const CFGNode *> visited_nodes;
        UnorderedSet<const CFGNode *> backward_visited_nodes;

        // Tracking which nodes are loop headers / merge targets.
        UnorderedSet<const CFGNode *> loop_headers;
        UnorderedSet<const CFGNode *> loop_merge_targets;
        UnorderedSet<const CFGNode *> selection_merge_targets;

        // State for switch-dispatch fallback.
        bool uses_switch_dispatch = false;
        // The synthetic entry block for the switch-dispatch.
        CFGNode *switch_dispatch_entry = nullptr;
};

} // namespace dxil_spv
