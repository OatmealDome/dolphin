#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_IR.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <utility>

#include <fmt/format.h>
#include <fmt/ostream.h>

using AnyCallback = s32 (*)(const void* operands);

s32 BytecodeGeneric::LoadImm32(std::ostream& stream, const LoadImm32Operands& operands)
{
  const auto& [ir_context, target, imm] = operands;
  fmt::println(stream, "LoadImm32(target={}, imm=0x{:08x})", (u8)target, imm);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::TransferRegHostToGuest(std::ostream& stream, const TransferRegHostToGuestOperands& operands)
{
  const auto& [ppc_state, ir_context, host, guest] = operands;
  fmt::println(stream, "TransferRegHostToGuest(host={}, guest={})", (u8)host, guest);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::TransferRegGuestToHost(std::ostream& stream, const TransferRegGuestToHostOperands& operands)
{
  const auto& [ppc_state, ir_context, guest, host] = operands;
  fmt::println(stream, "TransferRegGuestToHost(guest={}, host={})", guest, (u8)host);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::TransferRegImm32ToGuest(std::ostream& stream, const TransferRegImm32ToGuestOperands& operands)
{
  const auto& [ppc_state, guest, imm] = operands;
  fmt::println(stream, "TransferRegImm32ToGuest(guest={}, imm=0x{:08x})", guest, imm);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeGeneric::Add(std::ostream& stream, const AddOperands& operands)
{
  const auto& [ir_context, dest, a, b] = operands;
  fmt::println(stream, "Add(dest={}, a={}, b={})", (u8)dest, (u8)a, (u8)b);
  return sizeof(AnyCallback) + sizeof(operands);
}
