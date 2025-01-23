// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter.h"

#include <bit>

#include "Common/Assert.h"
#include "Common/BitUtils.h"
#include "Common/CommonTypes.h"
#include "Common/MathUtil.h"

#include "Core/Core.h"
#include "Core/CoreTiming.h"
#include "Core/PowerPC/Interpreter/Interpreter.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_IR.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_RegCache.h"
#include "Core/PowerPC/JitCommon/DivUtils.h"
#include "Core/PowerPC/PPCTables.h"
#include "Core/PowerPC/PowerPC.h"

using namespace JitCommon;

void BytecodeInterpreter::addix(UGeckoInstruction inst)
{
  INSTRUCTION_START
  JITDISABLE(bJITIntegerOff);
  u32 d = inst.RD, a = inst.RA;

  u32 imm = (u32)(s32)inst.SIMM_16;
  if (inst.OPCD == 15)
  {
    imm <<= 16;
  }

  if (a)
  {
    if (gpr.IsImm(a))
    {
      gpr.SetImmediate(d, gpr.GetImm(a) + imm);
    }
    else
    {
      gpr.BindToRegister(d, d == a);

      auto scratch = gpr.GetReg();

      Write(BytecodeGeneric::LoadImm32, {m_ir_context, scratch, imm});
      Write(BytecodeGeneric::Add, {m_ir_context, gpr.R(d), gpr.R(a), scratch});
      
      gpr.Unlock(scratch);
    }
  }
  else
  {
    // a == 0, implies zero register
    gpr.SetImmediate(d, imm);
  }
}
