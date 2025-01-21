// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fmt/ostream.h>

#include "Common/CommonTypes.h"

#include "Core/PowerPC/PowerPC.h"

namespace BytecodeGen
{
enum class BytecodeReg : u8
{
  R0 = 0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,

  Invalid = 0xFF,
};

struct BytecodeContext
{
  u32 regs[8];
};
}  // namespace BytecodeGen

namespace BytecodeGeneric
{
  struct LoadImm32Operands;

  struct TransferRegHostToGuestOperands;
  struct TransferRegGuestToHostOperands;
  struct TransferRegImm32ToGuestOperands;

  struct AddOperands;

  s32 LoadImm32(const LoadImm32Operands& operands);

  s32 TransferRegHostToGuest(const TransferRegHostToGuestOperands& operands);
  s32 TransferRegGuestToHost(const TransferRegGuestToHostOperands& operands);
  s32 TransferRegImm32ToGuest(const TransferRegImm32ToGuestOperands& operands);

  s32 Add(const AddOperands& operands);

  s32 LoadImm32(std::ostream& stream, const LoadImm32Operands& operands);
  s32 TransferRegHostToGuest(std::ostream& stream, const TransferRegHostToGuestOperands& operands);
  s32 TransferRegGuestToHost(std::ostream& stream, const TransferRegGuestToHostOperands& operands);
  s32 TransferRegImm32ToGuest(std::ostream& stream, const TransferRegImm32ToGuestOperands& operands);
  s32 Add(std::ostream& stream, const AddOperands& operands);
};

struct BytecodeGeneric::LoadImm32Operands
{
  BytecodeGen::BytecodeContext& ir_context;
  BytecodeGen::BytecodeReg target;
  u32 imm;
};

struct BytecodeGeneric::TransferRegHostToGuestOperands
{
  PowerPC::PowerPCState& ppc_state;
  BytecodeGen::BytecodeContext& ir_context;
  BytecodeGen::BytecodeReg host;
  size_t guest;
};

struct BytecodeGeneric::TransferRegGuestToHostOperands
{
  PowerPC::PowerPCState& ppc_state;
  BytecodeGen::BytecodeContext& ir_context;
  size_t guest;
  BytecodeGen::BytecodeReg host;
};

struct BytecodeGeneric::TransferRegImm32ToGuestOperands
{
  PowerPC::PowerPCState& ppc_state;
  size_t guest;
  u32 imm;
};

struct BytecodeGeneric::AddOperands
{
  BytecodeGen::BytecodeContext& ir_context;
  BytecodeGen::BytecodeReg dest;
  BytecodeGen::BytecodeReg a;
  BytecodeGen::BytecodeReg b;
};
