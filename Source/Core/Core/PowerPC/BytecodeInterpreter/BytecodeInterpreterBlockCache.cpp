// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreterBlockCache.h"

#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreterEmitter.h"
#include "Core/PowerPC/JitCommon/JitBase.h"

BytecodeInterpreterBlockCache::BytecodeInterpreterBlockCache(JitBase& jit) : JitBaseBlockCache{jit}
{
}

void BytecodeInterpreterBlockCache::Init()
{
  JitBaseBlockCache::Init();
  ClearRangesToFree();
}

void BytecodeInterpreterBlockCache::DestroyBlock(JitBlock& block)
{
  JitBaseBlockCache::DestroyBlock(block);

  if (block.near_begin != block.near_end)
    m_ranges_to_free_on_next_codegen.emplace_back(block.near_begin, block.near_end);
}

void BytecodeInterpreterBlockCache::ClearRangesToFree()
{
  m_ranges_to_free_on_next_codegen.clear();
}

void BytecodeInterpreterBlockCache::WriteLinkBlock(const JitBlock::LinkData& source,
                                                 const JitBlock* dest)
{
}

void BytecodeInterpreterBlockCache::WriteDestroyBlock(const JitBlock& block)
{
  BytecodeInterpreterEmitter emitter(block.normalEntry, block.near_end);
  emitter.Write(BytecodeInterpreterEmitter::PoisonCallback);
}
