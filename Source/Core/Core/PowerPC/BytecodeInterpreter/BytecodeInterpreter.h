// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

#include <rangeset/rangesizeset.h>

#include "Common/CommonTypes.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_IR.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_RegCache.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreterBlockCache.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreterEmitter.h"
#include "Core/PowerPC/JitCommon/JitBase.h"
#include "Core/PowerPC/PPCAnalyst.h"

namespace CoreTiming
{
class CoreTimingManager;
}
namespace CPU
{
enum class State;
}
class Interpreter;

class BytecodeInterpreter : public JitBase, public BytecodeInterpreterCodeBlock
{
public:
  explicit BytecodeInterpreter(Core::System& system);
  BytecodeInterpreter(const BytecodeInterpreter&) = delete;
  BytecodeInterpreter(BytecodeInterpreter&&) = delete;
  BytecodeInterpreter& operator=(const BytecodeInterpreter&) = delete;
  BytecodeInterpreter& operator=(BytecodeInterpreter&&) = delete;
  ~BytecodeInterpreter();

  void Init() override;
  void Shutdown() override;

  bool HandleFault(uintptr_t access_address, SContext* ctx) override { return false; }
  void ClearCache() override;

  void Run() override;
  void SingleStep() override;

  void Jit(u32 address) override;
  void Jit(u32 address, bool clear_cache_and_retry_on_failure);
  bool DoJit(u32 address, JitBlock* b, u32 nextPC);

  void EraseSingleBlock(const JitBlock& block) override;
  std::vector<MemoryStats> GetMemoryStats() const override;

  static std::size_t Disassemble(const JitBlock& block, std::ostream& stream);

  std::size_t DisassembleNearCode(const JitBlock& block, std::ostream& stream) const override;
  std::size_t DisassembleFarCode(const JitBlock& block, std::ostream& stream) const override;

  JitBaseBlockCache* GetBlockCache() override { return &m_block_cache; }
  const char* GetName() const override { return "Bytecode Interpreter"; }
  const CommonAsmRoutinesBase* GetAsmRoutines() override { return nullptr; }

  using Instruction = void (BytecodeInterpreter::*)(UGeckoInstruction);
  void FallBackToInterpreter(UGeckoInstruction inst);

  void DynaRunTable4(UGeckoInstruction inst);
  void DynaRunTable19(UGeckoInstruction inst);
  void DynaRunTable31(UGeckoInstruction inst);
  void DynaRunTable59(UGeckoInstruction inst);
  void DynaRunTable63(UGeckoInstruction inst);

  BytecodeGen::BytecodeContext m_ir_context;

  BytecodeGPRCache gpr;

protected:
  void CompileInstruction(PPCAnalyst::CodeOp& op);

private:
  void ExecuteOneBlock();

  bool HandleFunctionHooking(u32 address);
  void WriteEndBlock();

  // Finds a free memory region and sets the code emitter to point at that region.
  // Returns false if no free memory region can be found.
  bool SetEmitterStateToFreeCodeRegion();

  void FreeRanges();
  void ResetFreeMemoryRanges();

  void LogGeneratedCode() const;

  struct StartProfiledBlockOperands;
  template <bool profiled>
  struct EndBlockOperands;
  struct WritePCOperands;
  struct InterpretOperands;
  struct CheckExceptionsOperands;
  struct HLEFunctionOperands;
  struct WriteBrokenBlockNPCOperands;
  struct CheckHaltOperands;
  struct CheckIdleOperands;

  static s32 StartProfiledBlock(const StartProfiledBlockOperands& operands);
  static s32 StartProfiledBlock(std::ostream& stream, const StartProfiledBlockOperands& operands);
  template <bool profiled>
  static s32 EndBlock(const EndBlockOperands<profiled>& operands);
  template <bool profiled>
  static s32 EndBlock(std::ostream& stream, const EndBlockOperands<profiled>& operands);
  static s32 WritePC(const WritePCOperands& operands);
  static s32 WritePC(std::ostream& stream, const WritePCOperands& operands);
  static s32 Interpret(const InterpretOperands& operands);
  static s32 Interpret(std::ostream& stream, const InterpretOperands& operands);
  static s32 CheckExceptions(const CheckExceptionsOperands& operands);
  static s32 CheckExceptions(std::ostream& stream, const CheckExceptionsOperands& operands);
  static s32 HLEFunction(const HLEFunctionOperands& operands);
  static s32 HLEFunction(std::ostream& stream, const HLEFunctionOperands& operands);
  static s32 WriteBrokenBlockNPC(const WriteBrokenBlockNPCOperands& operands);
  static s32 WriteBrokenBlockNPC(std::ostream& stream, const WriteBrokenBlockNPCOperands& operands);
  static s32 CheckFPU(const CheckHaltOperands& operands);
  static s32 CheckFPU(std::ostream& stream, const CheckHaltOperands& operands);
  static s32 CheckBreakpoint(const CheckHaltOperands& operands);
  static s32 CheckBreakpoint(std::ostream& stream, const CheckHaltOperands& operands);
  static s32 CheckIdle(const CheckIdleOperands& operands);
  static s32 CheckIdle(std::ostream& stream, const CheckIdleOperands& operands);

  HyoutaUtilities::RangeSizeSet<u8*> m_free_ranges;
  BytecodeInterpreterBlockCache m_block_cache;
};

struct BytecodeInterpreter::StartProfiledBlockOperands
{
  JitBlock::ProfileData* profile_data;
};

template <>
struct BytecodeInterpreter::EndBlockOperands<false>
{
  PowerPC::PowerPCState& ppc_state;
  u32 downcount;
  u32 num_load_stores;
  u32 num_fp_inst;
  u32 : 32;
};

template <>
struct BytecodeInterpreter::EndBlockOperands<true> : BytecodeInterpreter::EndBlockOperands<false>
{
  JitBlock::ProfileData* profile_data;
};

struct BytecodeInterpreter::WritePCOperands
{
  PowerPC::PowerPCState& ppc_state;
  u32 current_pc;
  u32 : 32;
};

struct BytecodeInterpreter::InterpretOperands
{
  Interpreter& interpreter;
  void (*func)(Interpreter&, UGeckoInstruction);  // Interpreter::Instruction
  UGeckoInstruction inst;
};

struct BytecodeInterpreter::CheckExceptionsOperands
{
  PowerPC::PowerPCState& ppc_state;
  PowerPC::PowerPCManager& power_pc;
  u32 current_pc;
  u32 downcount;
};

struct BytecodeInterpreter::HLEFunctionOperands
{
  PowerPC::PowerPCState& ppc_state;
  Core::System& system;
  u32 current_pc;
  u32 hook_index;
};

struct BytecodeInterpreter::WriteBrokenBlockNPCOperands
{
  PowerPC::PowerPCState& ppc_state;
  u32 current_pc;
  u32 : 32;
};

struct BytecodeInterpreter::CheckHaltOperands
{
  PowerPC::PowerPCState& ppc_state;
  PowerPC::PowerPCManager& power_pc;
  u32 current_pc;
  u32 downcount;
};

struct BytecodeInterpreter::CheckIdleOperands
{
  PowerPC::PowerPCState& ppc_state;
  CoreTiming::CoreTimingManager& core_timing;
  u32 idle_pc;
};
