// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include "Common/CommonTypes.h"
#include "Core/PowerPC/Gekko.h"
#include "Core/PowerPC/PPCAnalyst.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter_IR.h"
#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreterEmitter.h"

class BytecodeInterpreter;

enum class RegType
{
  NotLoaded,
  Register,
  Immediate,
};

class OpArg
{
public:
  OpArg() = default;

  RegType GetType() const { return m_type; }
  BytecodeGen::BytecodeReg GetReg() const { return m_reg; }
  u32 GetImm() const { return m_value; }
  void Load(BytecodeGen::BytecodeReg reg, RegType type = RegType::Register)
  {
    m_type = type;
    m_reg = reg;
  }
  void LoadToImm(u32 imm)
  {
    m_type = RegType::Immediate;
    m_value = imm;

    m_reg = BytecodeGen::BytecodeReg::Invalid;
  }
  void Flush()
  {
    // Invalidate any previous information
    m_type = RegType::NotLoaded;
    m_reg = BytecodeGen::BytecodeReg::Invalid;

    // Arbitrarily large value that won't roll over on a lot of increments
    m_last_used = 0xFFFF;
  }

  u32 GetLastUsed() const { return m_last_used; }
  void ResetLastUsed() { m_last_used = 0; }
  void IncrementLastUsed() { ++m_last_used; }
  void SetDirty(bool dirty) { m_dirty = dirty; }
  bool IsDirty() const { return m_dirty; }

private:
  RegType m_type = RegType::NotLoaded;                         // store type
  u32 m_last_used = 0;
  bool m_dirty = false;

  // For Register
  BytecodeGen::BytecodeReg m_reg = BytecodeGen::BytecodeReg::Invalid;  // host register we are in

  // For Immediate
  u32 m_value = 0;  // IMM value
};

class HostReg
{
public:
  HostReg() = default;
  HostReg(BytecodeGen::BytecodeReg reg) : m_reg(reg) {}

  bool IsLocked() const { return m_locked; }
  void Lock() { m_locked = true; }
  void Unlock() { m_locked = false; }
  BytecodeGen::BytecodeReg GetReg() const { return m_reg; }

private:
  BytecodeGen::BytecodeReg m_reg = BytecodeGen::BytecodeReg::Invalid;
  bool m_locked = false;
};

class BytecodeRegCache
{
public:
  explicit BytecodeRegCache(size_t guest_reg_count) : m_guest_registers(guest_reg_count) {}
  virtual ~BytecodeRegCache() = default;

  void Init(BytecodeInterpreter* jit);

  // Returns a temporary register for use
  // Requires unlocking after done
  BytecodeGen::BytecodeReg GetReg();

  void UpdateLastUsed(BitSet32 regs_used);

  // Get available host registers
  u32 GetUnlockedRegisterCount() const;

  // Locks a register so a cache cannot use it
  // Useful for function calls
  template <typename T = BytecodeGen::BytecodeReg, typename... Args>
  void Lock(Args... args)
  {
    for (T reg : {args...})
    {
      FlushByHost(reg);
      LockRegister(reg);
    }
  }

  // Unlocks a locked register
  // Unlocks registers locked with both GetReg and LockRegister
  template <typename T = BytecodeGen::BytecodeReg, typename... Args>
  void Unlock(Args... args)
  {
    for (T reg : {args...})
    {
      FlushByHost(reg);
      UnlockRegister(reg);
    }
  }

protected:
  // Get the order of the host registers
  virtual void GetAllocationOrder() = 0;

  // Flushes the most stale register
  void FlushMostStaleRegister();

  // Lock a register
  void LockRegister(BytecodeGen::BytecodeReg host_reg);

  // Unlock a register
  void UnlockRegister(BytecodeGen::BytecodeReg host_reg);

  // Flushes a guest register by host provided
  virtual void FlushByHost(BytecodeGen::BytecodeReg host_reg) = 0;

  virtual void FlushRegister(size_t preg) = 0;

  void IncrementAllUsed()
  {
    for (auto& reg : m_guest_registers)
      reg.IncrementLastUsed();
  }

  BytecodeInterpreter* m_jit = nullptr;

  // Code emitter
  BytecodeInterpreterEmitter* m_emit = nullptr;

  // Host side registers that hold the host registers in order of use
  std::vector<HostReg> m_host_registers;

  // Our guest GPRs
  // PowerPC has 32 GPRs and 8 CRs
  // PowerPC also has 32 paired FPRs
  std::vector<OpArg> m_guest_registers;
};

class BytecodeGPRCache : public BytecodeRegCache
{
public:
  BytecodeGPRCache();

  // Returns a guest GPR inside of a host register.
  // Will dump an immediate to the host register as well.
  BytecodeGen::BytecodeReg R(size_t preg);

  // Set a register to an immediate. Only valid for guest GPRs.
  void SetImmediate(size_t preg, u32 imm, bool dirty = true);

  // Returns if a register is set as an immediate. Only valid for guest GPRs.
  bool IsImm(size_t preg) const { return m_guest_registers[preg].GetType() == RegType::Immediate; }

  // Gets the immediate that a register is set to. Only valid for guest GPRs.
  u32 GetImm(size_t preg) const { return m_guest_registers[preg].GetImm(); }

  bool IsImm(size_t preg, u32 imm) { return IsImm(preg) && GetImm(preg) == imm; }

  void BindToRegister(size_t guest_reg, bool will_read, bool will_write = true);

  void FlushRegisters(BitSet32 regs);

protected:
  // Get the order of the host registers
  void GetAllocationOrder() override;

  void FlushRegister(size_t index) override;

  void FlushByHost (BytecodeGen::BytecodeReg) override;
};
