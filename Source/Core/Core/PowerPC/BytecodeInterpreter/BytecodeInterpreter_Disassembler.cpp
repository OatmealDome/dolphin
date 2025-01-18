// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <utility>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "Core/HLE/HLE.h"

s32 BytecodeInterpreterEmitter::PoisonCallback(std::ostream& stream, const void* operands)
{
  stream << "PoisonCallback()\n";
  return sizeof(AnyCallback);
}

s32 BytecodeInterpreter::StartProfiledBlock(std::ostream& stream,
                                          const StartProfiledBlockOperands& operands)
{
  stream << "StartProfiledBlock()\n";
  return sizeof(AnyCallback) + sizeof(operands);
}

template <bool profiled>
s32 BytecodeInterpreter::EndBlock(std::ostream& stream, const EndBlockOperands<profiled>& operands)
{
  fmt::println(stream, "EndBlock<profiled={}>(downcount={}, num_load_stores={}, num_fp_inst={})",
               profiled, operands.downcount, operands.num_load_stores, operands.num_fp_inst);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::WritePC(std::ostream& stream,
                                const WritePCOperands& operands)
{
  fmt::println(stream, "WritePC(current_pc=0x{:08x})", operands.current_pc);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::Interpret(std::ostream& stream, const InterpretOperands& operands)
{
  fmt::println(stream, "Interpret(inst=0x{:08x})", operands.inst.hex);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::CheckExceptions(std::ostream& stream,
                                       const CheckExceptionsOperands& operands)
{
  fmt::println(stream, "CheckExceptions(current_pc=0x{:08x}, downcount={})",
               operands.current_pc, operands.downcount);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::HLEFunction(std::ostream& stream, const HLEFunctionOperands& operands)
{
  const auto& [ppc_state, system, current_pc, hook_index] = operands;
  fmt::println(stream, "HLEFunction(current_pc=0x{:08x}, hook_index={}) [\"{}\"]", current_pc,
               hook_index, HLE::GetHookNameByIndex(hook_index));
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::WriteBrokenBlockNPC(std::ostream& stream,
                                           const WriteBrokenBlockNPCOperands& operands)
{
  const auto& [ppc_state, current_pc] = operands;
  fmt::println(stream, "WriteBrokenBlockNPC(current_pc=0x{:08x})", current_pc);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::CheckFPU(std::ostream& stream, const CheckHaltOperands& operands)
{
  const auto& [ppc_state, power_pc, current_pc, downcount] = operands;
  fmt::println(stream, "CheckFPU(current_pc=0x{:08x}, downcount={})", current_pc, downcount);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::CheckBreakpoint(std::ostream& stream, const CheckHaltOperands& operands)
{
  const auto& [ppc_state, power_pc, current_pc, downcount] = operands;
  fmt::println(stream, "CheckBreakpoint(current_pc=0x{:08x}, downcount={})", current_pc, downcount);
  return sizeof(AnyCallback) + sizeof(operands);
}

s32 BytecodeInterpreter::CheckIdle(std::ostream& stream, const CheckIdleOperands& operands)
{
  const auto& [ppc_state, core_timing, idle_pc] = operands;
  fmt::println(stream, "CheckIdle(idle_pc=0x{:08x})", idle_pc);
  return sizeof(AnyCallback) + sizeof(operands);
}

static std::once_flag s_sorted_lookup_flag;

std::size_t BytecodeInterpreter::Disassemble(const JitBlock& block, std::ostream& stream)
{
  using LookupKV = std::pair<AnyCallback, AnyDisassemble>;

  // clang-format off
#define LOOKUP_KV(...) {AnyCallbackCast(__VA_ARGS__), AnyDisassembleCast(__VA_ARGS__)}
  // clang-format on

  // Function addresses aren't known at compile-time, so this array is sorted at run-time.
  static auto sorted_lookup = std::to_array<LookupKV>({
      LOOKUP_KV(BytecodeInterpreter::PoisonCallback),
      LOOKUP_KV(BytecodeInterpreter::StartProfiledBlock),
      LOOKUP_KV(BytecodeInterpreter::EndBlock<false>),
      LOOKUP_KV(BytecodeInterpreter::EndBlock<true>),
      LOOKUP_KV(BytecodeInterpreter::WritePC),
      LOOKUP_KV(BytecodeInterpreter::Interpret),
      LOOKUP_KV(BytecodeInterpreter::CheckExceptions),
      LOOKUP_KV(BytecodeInterpreter::HLEFunction),
      LOOKUP_KV(BytecodeInterpreter::WriteBrokenBlockNPC),
      LOOKUP_KV(BytecodeInterpreter::CheckFPU),
      LOOKUP_KV(BytecodeInterpreter::CheckBreakpoint),
      LOOKUP_KV(BytecodeInterpreter::CheckIdle),
  });

#undef LOOKUP_KV

  std::call_once(s_sorted_lookup_flag, []() {
    const auto end = std::ranges::sort(sorted_lookup, {}, &LookupKV::first);
    ASSERT_MSG(DYNA_REC, std::ranges::adjacent_find(sorted_lookup, {}, &LookupKV::first) == end,
               "Sorted lookup should not contain duplicate keys.");
  });

  std::size_t instruction_count = 0;
  for (const u8* normal_entry = block.normalEntry; normal_entry != block.near_end;
       ++instruction_count)
  {
    const auto callback = *reinterpret_cast<const AnyCallback*>(normal_entry);
    const auto kv = std::ranges::lower_bound(sorted_lookup, callback, {}, &LookupKV::first);
    if (kv != sorted_lookup.end() && kv->first == callback)
    {
      normal_entry += kv->second(stream, normal_entry + sizeof(AnyCallback));
      continue;
    }
    stream << "UNKNOWN OR ILLEGAL CALLBACK\n";
    break;
  }
  return instruction_count;
}
