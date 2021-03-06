/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include "nir.h"
#include "nir_builder.h"

struct lower_system_values_state {
   nir_builder builder;
   bool progress;
};

static bool
convert_block(nir_block *block, void *void_state)
{
   struct lower_system_values_state *state = void_state;

   nir_builder *b = &state->builder;

   nir_foreach_instr_safe(block, instr) {
      if (instr->type != nir_instr_type_intrinsic)
         continue;

      nir_intrinsic_instr *load_var = nir_instr_as_intrinsic(instr);

      if (load_var->intrinsic != nir_intrinsic_load_var)
         continue;

      nir_variable *var = load_var->variables[0]->var;
      if (var->data.mode != nir_var_system_value)
         continue;

      b->cursor = nir_after_instr(&load_var->instr);

      nir_ssa_def *sysval;
      switch (var->data.location) {
      case SYSTEM_VALUE_GLOBAL_INVOCATION_ID: {
         /* From the GLSL man page for gl_GlobalInvocationID:
          *
          *    "The value of gl_GlobalInvocationID is equal to
          *    gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID"
          */

         nir_const_value local_size;
         local_size.u32[0] = b->shader->info.cs.local_size[0];
         local_size.u32[1] = b->shader->info.cs.local_size[1];
         local_size.u32[2] = b->shader->info.cs.local_size[2];

         nir_ssa_def *group_id =
            nir_load_system_value(b, nir_intrinsic_load_work_group_id, 0);
         nir_ssa_def *local_id =
            nir_load_system_value(b, nir_intrinsic_load_local_invocation_id, 0);

         sysval = nir_iadd(b, nir_imul(b, group_id,
                                          nir_build_imm(b, 3, local_size)),
                              local_id);
         break;
      }

      case SYSTEM_VALUE_LOCAL_INVOCATION_INDEX: {
         /* From the GLSL man page for gl_LocalInvocationIndex:
          *
          *    "The value of gl_LocalInvocationIndex is equal to
          *    gl_LocalInvocationID.z * gl_WorkGroupSize.x *
          *    gl_WorkGroupSize.y + gl_LocalInvocationID.y *
          *    gl_WorkGroupSize.x + gl_LocalInvocationID.x"
          */
         nir_ssa_def *local_id =
            nir_load_system_value(b, nir_intrinsic_load_local_invocation_id, 0);

         nir_ssa_def *size_x = nir_imm_int(b, b->shader->info.cs.local_size[0]);
         nir_ssa_def *size_y = nir_imm_int(b, b->shader->info.cs.local_size[1]);

         sysval = nir_imul(b, nir_channel(b, local_id, 2),
                              nir_imul(b, size_x, size_y));
         sysval = nir_iadd(b, sysval,
                              nir_imul(b, nir_channel(b, local_id, 1), size_x));
         sysval = nir_iadd(b, sysval, nir_channel(b, local_id, 0));
         break;
      }

      case SYSTEM_VALUE_VERTEX_ID:
         if (b->shader->options->vertex_id_zero_based) {
            sysval = nir_iadd(b,
               nir_load_system_value(b, nir_intrinsic_load_vertex_id_zero_base, 0),
               nir_load_system_value(b, nir_intrinsic_load_base_vertex, 0));
         } else {
            sysval = nir_load_system_value(b, nir_intrinsic_load_vertex_id, 0);
         }
         break;

      case SYSTEM_VALUE_INSTANCE_INDEX:
         sysval = nir_iadd(b,
            nir_load_system_value(b, nir_intrinsic_load_instance_id, 0),
            nir_load_system_value(b, nir_intrinsic_load_base_instance, 0));
         break;

      default: {
         nir_intrinsic_op sysval_op =
            nir_intrinsic_from_system_value(var->data.location);
         sysval = nir_load_system_value(b, sysval_op, 0);
         break;
      } /* default */
      }

      nir_ssa_def_rewrite_uses(&load_var->dest.ssa, nir_src_for_ssa(sysval));
      nir_instr_remove(&load_var->instr);

      state->progress = true;
   }

   return true;
}

static bool
convert_impl(nir_function_impl *impl)
{
   struct lower_system_values_state state;

   state.progress = false;
   nir_builder_init(&state.builder, impl);

   nir_foreach_block(impl, convert_block, &state);
   nir_metadata_preserve(impl, nir_metadata_block_index |
                               nir_metadata_dominance);
   return state.progress;
}

bool
nir_lower_system_values(nir_shader *shader)
{
   bool progress = false;

   nir_foreach_function(shader, function) {
      if (function->impl)
         progress = convert_impl(function->impl) || progress;
   }

   exec_list_make_empty(&shader->system_values);

   return progress;
}
