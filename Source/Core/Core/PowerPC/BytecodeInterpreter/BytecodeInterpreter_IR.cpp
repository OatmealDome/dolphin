#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter.h"

#include "Common/CommonTypes.h"
#include "Common/Logging/Log.h"
#include "Core/PowerPC/PPCAnalyst.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/System.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreterEmitter.h"

using AnyCallback = s32 (*)(const void* operands);

s32 BytecodeGeneric::LoadImm32(const LoadImm32Operands& operands)
{
  auto& [ir_context, target, imm] = operands;
  ir_context.regs[(u8)target] = imm;

  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::TransferRegHostToGuest(const TransferRegHostToGuestOperands& operands)
{
  auto& [ppc_state, ir_context, host, guest] = operands;
  ppc_state.gpr[guest] = ir_context.regs[(u8)host];

  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::TransferRegGuestToHost(const TransferRegGuestToHostOperands& operands)
{
  auto& [ppc_state, ir_context, guest, host] = operands;
  ir_context.regs[(u8)host] = ppc_state.gpr[guest];

  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::TransferRegImm32ToGuest(const TransferRegImm32ToGuestOperands& operands)
{
  auto& [ppc_state, guest, imm] = operands;
  ppc_state.gpr[guest] = imm;

  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::Add(const AddOperands& operands)
{
  auto& [ir_context, dest, a, b] = operands;
  ir_context.regs[(u8)dest] = ir_context.regs[(u8)a] + ir_context.regs[(u8)b];

  return sizeof(AnyCallback) + sizeof(operands);
}
