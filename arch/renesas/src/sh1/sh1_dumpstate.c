/****************************************************************************
 * arch/renesas/src/sh1/sh1_dumpstate.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include "up_arch.h"
#include "up_internal.h"
#include "sched/sched.h"

#ifdef CONFIG_ARCH_STACKDUMP

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint32_t s_last_regs[XCPTCONTEXT_REGS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sh1_stackdump
 ****************************************************************************/

static void sh1_stackdump(uint32_t sp, uint32_t stack_base)
{
  uint32_t stack ;

  for (stack = sp & ~0x1f; stack < stack_base; stack += 32)
    {
      uint32_t *ptr = (uint32_t *)stack;
      _alert("%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
             stack, ptr[0], ptr[1], ptr[2], ptr[3],
             ptr[4], ptr[5], ptr[6], ptr[7]);
    }
}

/****************************************************************************
 * Name: sh1_registerdump
 ****************************************************************************/

static inline void sh1_registerdump(void)
{
  uint32_t *ptr = (uint32_t *)g_current_regs;

  /* Are user registers available from interrupt processing? */

  if (ptr == NULL)
    {
      /* No.. capture user registers by hand */

      up_saveusercontext(s_last_regs);
      regs = s_last_regs;
    }

  /* Dump the interrupt registers */

  _alert("PC: %08x SR=%08x\n",
        ptr[REG_PC], ptr[REG_SR]);

  _alert("PR: %08x GBR: %08x MACH: %08x MACL: %08x\n",
        ptr[REG_PR], ptr[REG_GBR], ptr[REG_MACH], ptr[REG_MACL]);

  _alert("R%d: %08x %08x %08x %08x %08x %08x %08x %08x\n", 0,
        ptr[REG_R0], ptr[REG_R1], ptr[REG_R2], ptr[REG_R3],
        ptr[REG_R4], ptr[REG_R5], ptr[REG_R6], ptr[REG_R7]);

  _alert("R%d: %08x %08x %08x %08x %08x %08x %08x %08x\n", 8,
        ptr[REG_R8], ptr[REG_R9], ptr[REG_R10], ptr[REG_R11],
        ptr[REG_R12], ptr[REG_R13], ptr[REG_R14], ptr[REG_R15]);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_dumpstate
 ****************************************************************************/

void up_dumpstate(void)
{
  struct tcb_s *rtcb = running_task();
  uint32_t sp = renesas_getsp();
  uint32_t ustackbase;
  uint32_t ustacksize;
#if CONFIG_ARCH_INTERRUPTSTACK > 3
  uint32_t istackbase;
  uint32_t istacksize;
#endif

  /* Dump the registers (if available) */

  sh1_registerdump();

  /* Get the limits on the user stack memory */

  ustackbase = (uint32_t)rtcb->adj_stack_ptr;
  ustacksize = (uint32_t)rtcb->adj_stack_size;

  /* Get the limits on the interrupt stack memory */

#if CONFIG_ARCH_INTERRUPTSTACK > 3
  istackbase = (uint32_t)&g_intstackbase;
  istacksize = (CONFIG_ARCH_INTERRUPTSTACK & ~3);

  /* Show interrupt stack info */

  _alert("sp:     %08x\n", sp);
  _alert("IRQ stack:\n");
  _alert("  base: %08x\n", istackbase);
  _alert("  size: %08x\n", istacksize);

  /* Does the current stack pointer lie within the interrupt
   * stack?
   */

  if (sp < istackbase && sp >= istackbase - istacksize)
    {
      /* Yes.. dump the interrupt stack */

      sh1_stackdump(sp, istackbase);

      /* Extract the user stack pointer which should lie
       * at the base of the interrupt stack.
       */

      sp = g_intstackbase;
      _alert("sp:     %08x\n", sp);
    }
  else if (g_current_regs)
    {
      _alert("ERROR: Stack pointer is not within the interrupt stack\n");
      sh1_stackdump(istackbase - istacksize, istackbase);
    }

  /* Show user stack info */

  _alert("User stack:\n");
  _alert("  base: %08x\n", ustackbase);
  _alert("  size: %08x\n", ustacksize);
#else
  _alert("sp:         %08x\n", sp);
  _alert("stack base: %08x\n", ustackbase);
  _alert("stack size: %08x\n", ustacksize);
#endif

  /* Dump the user stack if the stack pointer lies within the allocated user
   * stack memory.
   */

  if (sp >= ustackbase || sp < ustackbase - ustacksize)
    {
      _alert("ERROR: Stack pointer is not within allocated stack\n");
      sh1_stackdump(ustackbase - ustacksize, ustackbase);
    }
  else
    {
      sh1_stackdump(sp, ustackbase);
    }
}

#endif /* CONFIG_ARCH_STACKDUMP */
