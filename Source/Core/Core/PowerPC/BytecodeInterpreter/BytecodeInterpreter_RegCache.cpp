// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_RegCache.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "Common/Assert.h"
#include "Common/BitSet.h"
#include "Common/CommonTypes.h"
#include "Common/Logging/Log.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter.h"

using namespace BytecodeGen;

void BytecodeRegCache::Init(BytecodeInterpreter* jit)
{
  m_jit = jit;
  m_emit = jit;
  GetAllocationOrder();
}

BytecodeReg BytecodeRegCache::GetReg()
{
  // If we have no registers left, dump the most stale register first
  if (GetUnlockedRegisterCount() == 0)
    FlushMostStaleRegister();

  for (auto& it : m_host_registers)
  {
    if (!it.IsLocked())
    {
      it.Lock();
      return it.GetReg();
    }
  }
  // Holy cow, how did you run out of registers?
  // We can't return anything reasonable in this case. Return Invalid and watch the failure
  // happen
  ASSERT_MSG(DYNA_REC, 0, "All available registers are locked!");
  return BytecodeReg::Invalid;
}

void BytecodeRegCache::UpdateLastUsed(BitSet32 regs_used)
{
  for (size_t i = 0; i < m_guest_registers.size(); ++i)
  {
    OpArg& reg = m_guest_registers[i];
    if (i < 32 && regs_used[i])
      reg.ResetLastUsed();
    else
      reg.IncrementLastUsed();
  }
}

u32 BytecodeRegCache::GetUnlockedRegisterCount() const
{
  u32 unlocked_registers = 0;
  for (const auto& it : m_host_registers)
  {
    if (!it.IsLocked())
      ++unlocked_registers;
  }
  return unlocked_registers;
}

void BytecodeRegCache::LockRegister(BytecodeReg host_reg)
{
  auto reg = std::ranges::find(m_host_registers, host_reg, &HostReg::GetReg);
  ASSERT_MSG(DYNA_REC, reg != m_host_registers.end(),
             "Don't try locking a register that isn't in the cache. Reg {}",
             static_cast<int>(host_reg));
  reg->Lock();
}

void BytecodeRegCache::UnlockRegister(BytecodeReg host_reg)
{
  auto reg = std::ranges::find(m_host_registers, host_reg, &HostReg::GetReg);
  ASSERT_MSG(DYNA_REC, reg != m_host_registers.end(),
             "Don't try unlocking a register that isn't in the cache. Reg {}",
             static_cast<int>(host_reg));
  reg->Unlock();
}

void BytecodeRegCache::FlushMostStaleRegister()
{
  size_t most_stale_preg = 0;
  u32 most_stale_amount = 0;

  for (size_t i = 0; i < m_guest_registers.size(); ++i)
  {
    const auto& reg = m_guest_registers[i];
    const u32 last_used = reg.GetLastUsed();

    if (last_used > most_stale_amount && reg.GetType() != RegType::NotLoaded &&
        reg.GetType() != RegType::Immediate)
    {
      most_stale_preg = i;
      most_stale_amount = last_used;
    }
  }

  FlushRegister(most_stale_preg);
}

BytecodeGPRCache::BytecodeGPRCache() : BytecodeRegCache(32)
{
}

void BytecodeGPRCache::FlushRegisters(BitSet32 regs)
{
  for (auto iter = regs.begin(); iter != regs.end(); ++iter)
  {
    const int i = *iter;
    FlushRegister(i);
  }
}

void BytecodeGPRCache::FlushRegister(size_t preg)
{
  OpArg& reg = m_guest_registers[preg];

  if (reg.GetType() == RegType::Register)
  {
    BytecodeReg host_reg = reg.GetReg();
    if (reg.IsDirty())
    {
      m_emit->Write(BytecodeGeneric::TransferRegHostToGuest, {m_jit->m_ppc_state, m_jit->m_ir_context, host_reg, preg});
    }

    UnlockRegister(host_reg);
  }
  else if (reg.GetType() == RegType::Immediate)
  {
    if (reg.IsDirty())
    {
      m_emit->Write(BytecodeGeneric::TransferRegImm32ToGuest, {m_jit->m_ppc_state, preg, reg.GetImm()});
    }
  }

  reg.Flush();
}

BytecodeReg BytecodeGPRCache::R(size_t preg)
{
  OpArg& reg = m_guest_registers[preg];

  IncrementAllUsed();
  reg.ResetLastUsed();

  switch (reg.GetType())
  {
  case RegType::Register:  // already in a reg
    return reg.GetReg();
  case RegType::Immediate:  // Is an immediate
  {
    BytecodeReg host_reg = GetReg();
    m_emit->Write(BytecodeGeneric::LoadImm32, {m_jit->m_ir_context, host_reg, reg.GetImm()});
    reg.Load(host_reg);
    return host_reg;
  }
  break;
  case RegType::NotLoaded:  // Register isn't loaded at /all/
  {
    // This is a bit annoying. We try to keep these preloaded as much as possible
    // This can also happen on cases where PPCAnalyst isn't feeing us proper register usage
    // statistics
    BytecodeReg host_reg = GetReg();
    reg.Load(host_reg);
    reg.SetDirty(false);
    m_emit->Write(BytecodeGeneric::TransferRegGuestToHost, {m_jit->m_ppc_state, m_jit->m_ir_context, preg, host_reg});
    return host_reg;
  }
  break;
  default:
    ERROR_LOG_FMT(DYNA_REC, "Invalid OpArg Type!");
    break;
  }
  // We've got an issue if we end up here
  return BytecodeReg::Invalid;
}

void BytecodeGPRCache::SetImmediate(size_t preg, u32 imm, bool dirty)
{
  OpArg& reg = m_guest_registers[preg];
  if (reg.GetType() == RegType::Register)
    UnlockRegister(reg.GetReg());
  reg.LoadToImm(imm);
  reg.SetDirty(dirty);
}

void BytecodeGPRCache::BindToRegister(const size_t preg, bool will_read, bool will_write)
{
  OpArg& reg = m_guest_registers[preg];

  reg.ResetLastUsed();

  const RegType reg_type = reg.GetType();
  if (reg_type == RegType::NotLoaded)
  {
    const BytecodeReg host_reg = GetReg();
    reg.Load(host_reg);
    reg.SetDirty(will_write);
    if (will_read)
    {
      m_emit->Write(BytecodeGeneric::TransferRegGuestToHost, {m_jit->m_ppc_state, m_jit->m_ir_context, preg, host_reg});
    }
  }
  else if (reg_type == RegType::Immediate)
  {
    const BytecodeReg host_reg = GetReg();
    if (will_read || !will_write)
    {
      // TODO: Emitting this instruction when (!will_read && !will_write) would be unnecessary if we
      // had some way to indicate to Flush that the immediate value should be written to ppcState
      // even though there is a host register allocated
      m_emit->Write(BytecodeGeneric::LoadImm32, {m_jit->m_ir_context, host_reg, reg.GetImm()});
    }
    reg.Load(host_reg);
    if (will_write)
      reg.SetDirty(true);
  }
  else if (will_write)
  {
    reg.SetDirty(true);
  }
}

void BytecodeGPRCache::GetAllocationOrder()
{
  static constexpr auto allocation_order = {
      BytecodeReg::R7,
      BytecodeReg::R6,
      BytecodeReg::R5,
      BytecodeReg::R4,
      BytecodeReg::R3,
      BytecodeReg::R2,
      BytecodeReg::R1,
      BytecodeReg::R0,
  };

  for (BytecodeReg reg : allocation_order)
    m_host_registers.push_back(HostReg(reg));
}

void BytecodeGPRCache::FlushByHost(BytecodeGen::BytecodeReg host_reg)
{
  for (size_t i = 0; i < m_guest_registers.size(); ++i)
  {
    const OpArg& reg = m_guest_registers[i];
    if (reg.GetType() == RegType::Register && reg.GetReg() == host_reg)
    {
      FlushRegister(i);
      return;
    }
  }
}

