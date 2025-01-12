// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Core/PowerPC/BytecodeInterpreter/BytecodeInterpreter.h"

#include <array>

#include "Common/Assert.h"
#include "Core/PowerPC/Gekko.h"

namespace
{
struct BytecodeInterpreterOpTemplate
{
  u32 opcode;
  BytecodeInterpreter::Instruction fn;
};

constexpr std::array<BytecodeInterpreterOpTemplate, 54> s_primary_table{{
    {4, &BytecodeInterpreter::FallBackToInterpreter},    // RunTable4
    {19, &BytecodeInterpreter::FallBackToInterpreter},  // RunTable19
    {31, &BytecodeInterpreter::FallBackToInterpreter},  // RunTable31
    {59, &BytecodeInterpreter::FallBackToInterpreter},  // RunTable59
    {63, &BytecodeInterpreter::FallBackToInterpreter},  // RunTable63

    {16, &BytecodeInterpreter::FallBackToInterpreter},  // bcx
    {18, &BytecodeInterpreter::FallBackToInterpreter},   // bx

    {3, &BytecodeInterpreter::FallBackToInterpreter},  // twi
    {17, &BytecodeInterpreter::FallBackToInterpreter},  // sc

    {7, &BytecodeInterpreter::FallBackToInterpreter},   // mulli
    {8, &BytecodeInterpreter::FallBackToInterpreter},  // subfic
    {10, &BytecodeInterpreter::FallBackToInterpreter},  // cmpli
    {11, &BytecodeInterpreter::FallBackToInterpreter},   // cmpi
    {12, &BytecodeInterpreter::FallBackToInterpreter},  // addic
    {13, &BytecodeInterpreter::FallBackToInterpreter},  // addic_rc
    {14, &BytecodeInterpreter::FallBackToInterpreter},  // addi
    {15, &BytecodeInterpreter::FallBackToInterpreter},  // addis

    {20, &BytecodeInterpreter::FallBackToInterpreter},  // rlwimix
    {21, &BytecodeInterpreter::FallBackToInterpreter},  // rlwinmx
    {23, &BytecodeInterpreter::FallBackToInterpreter},   // rlwnmx

    {24, &BytecodeInterpreter::FallBackToInterpreter},  // ori
    {25, &BytecodeInterpreter::FallBackToInterpreter},  // oris
    {26, &BytecodeInterpreter::FallBackToInterpreter},  // xori
    {27, &BytecodeInterpreter::FallBackToInterpreter},  // xoris
    {28, &BytecodeInterpreter::FallBackToInterpreter},  // andi_rc
    {29, &BytecodeInterpreter::FallBackToInterpreter},  // andis_rc

    {32, &BytecodeInterpreter::FallBackToInterpreter},  // lwz
    {33, &BytecodeInterpreter::FallBackToInterpreter},  // lwzu
    {34, &BytecodeInterpreter::FallBackToInterpreter},  // lbz
    {35, &BytecodeInterpreter::FallBackToInterpreter},  // lbzu
    {40, &BytecodeInterpreter::FallBackToInterpreter},  // lhz
    {41, &BytecodeInterpreter::FallBackToInterpreter},  // lhzu
    {42, &BytecodeInterpreter::FallBackToInterpreter},  // lha
    {43, &BytecodeInterpreter::FallBackToInterpreter},  // lhau

    {44, &BytecodeInterpreter::FallBackToInterpreter},  // sth
    {45, &BytecodeInterpreter::FallBackToInterpreter},  // sthu
    {36, &BytecodeInterpreter::FallBackToInterpreter},  // stw
    {37, &BytecodeInterpreter::FallBackToInterpreter},  // stwu
    {38, &BytecodeInterpreter::FallBackToInterpreter},  // stb
    {39, &BytecodeInterpreter::FallBackToInterpreter},  // stbu

    {46, &BytecodeInterpreter::FallBackToInterpreter},   // lmw
    {47, &BytecodeInterpreter::FallBackToInterpreter},  // stmw

    {48, &BytecodeInterpreter::FallBackToInterpreter},  // lfs
    {49, &BytecodeInterpreter::FallBackToInterpreter},  // lfsu
    {50, &BytecodeInterpreter::FallBackToInterpreter},  // lfd
    {51, &BytecodeInterpreter::FallBackToInterpreter},  // lfdu

    {52, &BytecodeInterpreter::FallBackToInterpreter},  // stfs
    {53, &BytecodeInterpreter::FallBackToInterpreter},  // stfsu
    {54, &BytecodeInterpreter::FallBackToInterpreter},  // stfd
    {55, &BytecodeInterpreter::FallBackToInterpreter},  // stfdu

    {56, &BytecodeInterpreter::FallBackToInterpreter},   // psq_l
    {57, &BytecodeInterpreter::FallBackToInterpreter},   // psq_lu
    {60, &BytecodeInterpreter::FallBackToInterpreter},  // psq_st
    {61, &BytecodeInterpreter::FallBackToInterpreter},  // psq_stu

    // missing: 0, 1, 2, 5, 6, 9, 22, 30, 58, 62
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 13> s_table4{{
    // SUBOP10
    {0, &BytecodeInterpreter::FallBackToInterpreter},      // ps_cmpu0
    {32, &BytecodeInterpreter::FallBackToInterpreter},     // ps_cmpo0
    {40, &BytecodeInterpreter::FallBackToInterpreter},     // ps_neg
    {136, &BytecodeInterpreter::FallBackToInterpreter},    // ps_nabs
    {264, &BytecodeInterpreter::FallBackToInterpreter},    // ps_abs
    {64, &BytecodeInterpreter::FallBackToInterpreter},     // ps_cmpu1
    {72, &BytecodeInterpreter::FallBackToInterpreter},     // ps_mr
    {96, &BytecodeInterpreter::FallBackToInterpreter},     // ps_cmpo1
    {528, &BytecodeInterpreter::FallBackToInterpreter},  // ps_merge00
    {560, &BytecodeInterpreter::FallBackToInterpreter},  // ps_merge01
    {592, &BytecodeInterpreter::FallBackToInterpreter},  // ps_merge10
    {624, &BytecodeInterpreter::FallBackToInterpreter},  // ps_merge11

    {1014, &BytecodeInterpreter::FallBackToInterpreter},  // dcbz_l
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 17> s_table4_2{{
    {10, &BytecodeInterpreter::FallBackToInterpreter},    // ps_sum0
    {11, &BytecodeInterpreter::FallBackToInterpreter},    // ps_sum1
    {12, &BytecodeInterpreter::FallBackToInterpreter},   // ps_muls0
    {13, &BytecodeInterpreter::FallBackToInterpreter},   // ps_muls1
    {14, &BytecodeInterpreter::FallBackToInterpreter},   // ps_madds0
    {15, &BytecodeInterpreter::FallBackToInterpreter},   // ps_madds1
    {18, &BytecodeInterpreter::FallBackToInterpreter},   // ps_div
    {20, &BytecodeInterpreter::FallBackToInterpreter},   // ps_sub
    {21, &BytecodeInterpreter::FallBackToInterpreter},   // ps_add
    {23, &BytecodeInterpreter::FallBackToInterpreter},     // ps_sel
    {24, &BytecodeInterpreter::FallBackToInterpreter},     // ps_res
    {25, &BytecodeInterpreter::FallBackToInterpreter},   // ps_mul
    {26, &BytecodeInterpreter::FallBackToInterpreter},  // ps_rsqrte
    {28, &BytecodeInterpreter::FallBackToInterpreter},   // ps_msub
    {29, &BytecodeInterpreter::FallBackToInterpreter},   // ps_madd
    {30, &BytecodeInterpreter::FallBackToInterpreter},   // ps_nmsub
    {31, &BytecodeInterpreter::FallBackToInterpreter},   // ps_nmadd
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 4> s_table4_3{{
    {6, &BytecodeInterpreter::FallBackToInterpreter},    // psq_lx
    {7, &BytecodeInterpreter::FallBackToInterpreter},   // psq_stx
    {38, &BytecodeInterpreter::FallBackToInterpreter},   // psq_lux
    {39, &BytecodeInterpreter::FallBackToInterpreter},  // psq_stux
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 13> s_table19{{
    {528, &BytecodeInterpreter::FallBackToInterpreter},  // bcctrx
    {16, &BytecodeInterpreter::FallBackToInterpreter},    // bclrx
    {257, &BytecodeInterpreter::FallBackToInterpreter},   // crand
    {129, &BytecodeInterpreter::FallBackToInterpreter},   // crandc
    {289, &BytecodeInterpreter::FallBackToInterpreter},   // creqv
    {225, &BytecodeInterpreter::FallBackToInterpreter},   // crnand
    {33, &BytecodeInterpreter::FallBackToInterpreter},    // crnor
    {449, &BytecodeInterpreter::FallBackToInterpreter},   // cror
    {417, &BytecodeInterpreter::FallBackToInterpreter},   // crorc
    {193, &BytecodeInterpreter::FallBackToInterpreter},   // crxor

    {150, &BytecodeInterpreter::FallBackToInterpreter},  // isync
    {0, &BytecodeInterpreter::FallBackToInterpreter},         // mcrf

    {50, &BytecodeInterpreter::FallBackToInterpreter},  // rfi
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 107> s_table31{{
    {266, &BytecodeInterpreter::FallBackToInterpreter},     // addx
    {778, &BytecodeInterpreter::FallBackToInterpreter},     // addox
    {10, &BytecodeInterpreter::FallBackToInterpreter},     // addcx
    {522, &BytecodeInterpreter::FallBackToInterpreter},    // addcox
    {138, &BytecodeInterpreter::FallBackToInterpreter},    // addex
    {650, &BytecodeInterpreter::FallBackToInterpreter},    // addeox
    {234, &BytecodeInterpreter::FallBackToInterpreter},    // addmex
    {746, &BytecodeInterpreter::FallBackToInterpreter},    // addmeox
    {202, &BytecodeInterpreter::FallBackToInterpreter},   // addzex
    {714, &BytecodeInterpreter::FallBackToInterpreter},   // addzeox
    {491, &BytecodeInterpreter::FallBackToInterpreter},    // divwx
    {1003, &BytecodeInterpreter::FallBackToInterpreter},   // divwox
    {459, &BytecodeInterpreter::FallBackToInterpreter},   // divwux
    {971, &BytecodeInterpreter::FallBackToInterpreter},   // divwuox
    {75, &BytecodeInterpreter::FallBackToInterpreter},    // mulhwx
    {11, &BytecodeInterpreter::FallBackToInterpreter},   // mulhwux
    {235, &BytecodeInterpreter::FallBackToInterpreter},   // mullwx
    {747, &BytecodeInterpreter::FallBackToInterpreter},   // mullwox
    {104, &BytecodeInterpreter::FallBackToInterpreter},     // negx
    {616, &BytecodeInterpreter::FallBackToInterpreter},     // negox
    {40, &BytecodeInterpreter::FallBackToInterpreter},     // subfx
    {552, &BytecodeInterpreter::FallBackToInterpreter},    // subfox
    {8, &BytecodeInterpreter::FallBackToInterpreter},     // subfcx
    {520, &BytecodeInterpreter::FallBackToInterpreter},   // subfcox
    {136, &BytecodeInterpreter::FallBackToInterpreter},   // subfex
    {648, &BytecodeInterpreter::FallBackToInterpreter},   // subfeox
    {232, &BytecodeInterpreter::FallBackToInterpreter},   // subfmex
    {744, &BytecodeInterpreter::FallBackToInterpreter},   // subfmeox
    {200, &BytecodeInterpreter::FallBackToInterpreter},  // subfzex
    {712, &BytecodeInterpreter::FallBackToInterpreter},  // subfzeox

    {28, &BytecodeInterpreter::FallBackToInterpreter},    // andx
    {60, &BytecodeInterpreter::FallBackToInterpreter},    // andcx
    {444, &BytecodeInterpreter::FallBackToInterpreter},   // orx
    {124, &BytecodeInterpreter::FallBackToInterpreter},   // norx
    {316, &BytecodeInterpreter::FallBackToInterpreter},   // xorx
    {412, &BytecodeInterpreter::FallBackToInterpreter},   // orcx
    {476, &BytecodeInterpreter::FallBackToInterpreter},   // nandx
    {284, &BytecodeInterpreter::FallBackToInterpreter},   // eqvx
    {0, &BytecodeInterpreter::FallBackToInterpreter},       // cmp
    {32, &BytecodeInterpreter::FallBackToInterpreter},     // cmpl
    {26, &BytecodeInterpreter::FallBackToInterpreter},  // cntlzwx
    {922, &BytecodeInterpreter::FallBackToInterpreter},  // extshx
    {954, &BytecodeInterpreter::FallBackToInterpreter},  // extsbx
    {536, &BytecodeInterpreter::FallBackToInterpreter},    // srwx
    {792, &BytecodeInterpreter::FallBackToInterpreter},   // srawx
    {824, &BytecodeInterpreter::FallBackToInterpreter},  // srawix
    {24, &BytecodeInterpreter::FallBackToInterpreter},     // slwx

    {54, &BytecodeInterpreter::FallBackToInterpreter},        // dcbst
    {86, &BytecodeInterpreter::FallBackToInterpreter},        // dcbf
    {246, &BytecodeInterpreter::FallBackToInterpreter},       // dcbtst
    {278, &BytecodeInterpreter::FallBackToInterpreter},       // dcbt
    {470, &BytecodeInterpreter::FallBackToInterpreter},       // dcbi
    {758, &BytecodeInterpreter::FallBackToInterpreter},  // dcba
    {1014, &BytecodeInterpreter::FallBackToInterpreter},      // dcbz

    // load word
    {23, &BytecodeInterpreter::FallBackToInterpreter},  // lwzx
    {55, &BytecodeInterpreter::FallBackToInterpreter},  // lwzux

    // load halfword
    {279, &BytecodeInterpreter::FallBackToInterpreter},  // lhzx
    {311, &BytecodeInterpreter::FallBackToInterpreter},  // lhzux

    // load halfword signextend
    {343, &BytecodeInterpreter::FallBackToInterpreter},  // lhax
    {375, &BytecodeInterpreter::FallBackToInterpreter},  // lhaux

    // load byte
    {87, &BytecodeInterpreter::FallBackToInterpreter},   // lbzx
    {119, &BytecodeInterpreter::FallBackToInterpreter},  // lbzux

    // load byte reverse
    {534, &BytecodeInterpreter::FallBackToInterpreter},  // lwbrx
    {790, &BytecodeInterpreter::FallBackToInterpreter},  // lhbrx

    // Conditional load/store (Wii SMP)
    {150, &BytecodeInterpreter::FallBackToInterpreter},  // stwcxd
    {20, &BytecodeInterpreter::FallBackToInterpreter},   // lwarx

    // load string (interpret these)
    {533, &BytecodeInterpreter::FallBackToInterpreter},  // lswx
    {597, &BytecodeInterpreter::FallBackToInterpreter},  // lswi

    // store word
    {151, &BytecodeInterpreter::FallBackToInterpreter},  // stwx
    {183, &BytecodeInterpreter::FallBackToInterpreter},  // stwux

    // store halfword
    {407, &BytecodeInterpreter::FallBackToInterpreter},  // sthx
    {439, &BytecodeInterpreter::FallBackToInterpreter},  // sthux

    // store byte
    {215, &BytecodeInterpreter::FallBackToInterpreter},  // stbx
    {247, &BytecodeInterpreter::FallBackToInterpreter},  // stbux

    // store bytereverse
    {662, &BytecodeInterpreter::FallBackToInterpreter},  // stwbrx
    {918, &BytecodeInterpreter::FallBackToInterpreter},  // sthbrx

    {661, &BytecodeInterpreter::FallBackToInterpreter},  // stswx
    {725, &BytecodeInterpreter::FallBackToInterpreter},  // stswi

    // fp load/store
    {535, &BytecodeInterpreter::FallBackToInterpreter},  // lfsx
    {567, &BytecodeInterpreter::FallBackToInterpreter},  // lfsux
    {599, &BytecodeInterpreter::FallBackToInterpreter},  // lfdx
    {631, &BytecodeInterpreter::FallBackToInterpreter},  // lfdux

    {663, &BytecodeInterpreter::FallBackToInterpreter},  // stfsx
    {695, &BytecodeInterpreter::FallBackToInterpreter},  // stfsux
    {727, &BytecodeInterpreter::FallBackToInterpreter},  // stfdx
    {759, &BytecodeInterpreter::FallBackToInterpreter},  // stfdux
    {983, &BytecodeInterpreter::FallBackToInterpreter},  // stfiwx

    {19, &BytecodeInterpreter::FallBackToInterpreter},     // mfcr
    {83, &BytecodeInterpreter::FallBackToInterpreter},    // mfmsr
    {144, &BytecodeInterpreter::FallBackToInterpreter},   // mtcrf
    {146, &BytecodeInterpreter::FallBackToInterpreter},   // mtmsr
    {210, &BytecodeInterpreter::FallBackToInterpreter},    // mtsr
    {242, &BytecodeInterpreter::FallBackToInterpreter},  // mtsrin
    {339, &BytecodeInterpreter::FallBackToInterpreter},   // mfspr
    {467, &BytecodeInterpreter::FallBackToInterpreter},   // mtspr
    {371, &BytecodeInterpreter::FallBackToInterpreter},    // mftb
    {512, &BytecodeInterpreter::FallBackToInterpreter},   // mcrxr
    {595, &BytecodeInterpreter::FallBackToInterpreter},    // mfsr
    {659, &BytecodeInterpreter::FallBackToInterpreter},  // mfsrin

    {4, &BytecodeInterpreter::FallBackToInterpreter},                      // tw
    {598, &BytecodeInterpreter::FallBackToInterpreter},              // sync
    {982, &BytecodeInterpreter::FallBackToInterpreter},  // icbi

    // Unused instructions on GC
    {310, &BytecodeInterpreter::FallBackToInterpreter},  // eciwx
    {438, &BytecodeInterpreter::FallBackToInterpreter},  // ecowx
    {854, &BytecodeInterpreter::FallBackToInterpreter},                  // eieio
    {306, &BytecodeInterpreter::FallBackToInterpreter},  // tlbie
    {566, &BytecodeInterpreter::FallBackToInterpreter},              // tlbsync
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 9> s_table59{{
    {18, &BytecodeInterpreter::FallBackToInterpreter},  // fdivsx
    {20, &BytecodeInterpreter::FallBackToInterpreter},  // fsubsx
    {21, &BytecodeInterpreter::FallBackToInterpreter},  // faddsx
    {24, &BytecodeInterpreter::FallBackToInterpreter},     // fresx
    {25, &BytecodeInterpreter::FallBackToInterpreter},  // fmulsx
    {28, &BytecodeInterpreter::FallBackToInterpreter},  // fmsubsx
    {29, &BytecodeInterpreter::FallBackToInterpreter},  // fmaddsx
    {30, &BytecodeInterpreter::FallBackToInterpreter},  // fnmsubsx
    {31, &BytecodeInterpreter::FallBackToInterpreter},  // fnmaddsx
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 15> s_table63{{
    {264, &BytecodeInterpreter::FallBackToInterpreter},  // fabsx
    {32, &BytecodeInterpreter::FallBackToInterpreter},      // fcmpo
    {0, &BytecodeInterpreter::FallBackToInterpreter},       // fcmpu
    {14, &BytecodeInterpreter::FallBackToInterpreter},     // fctiwx
    {15, &BytecodeInterpreter::FallBackToInterpreter},     // fctiwzx
    {72, &BytecodeInterpreter::FallBackToInterpreter},   // fmrx
    {136, &BytecodeInterpreter::FallBackToInterpreter},  // fnabsx
    {40, &BytecodeInterpreter::FallBackToInterpreter},   // fnegx
    {12, &BytecodeInterpreter::FallBackToInterpreter},      // frspx

    {64, &BytecodeInterpreter::FallBackToInterpreter},     // mcrfs
    {583, &BytecodeInterpreter::FallBackToInterpreter},    // mffsx
    {70, &BytecodeInterpreter::FallBackToInterpreter},   // mtfsb0x
    {38, &BytecodeInterpreter::FallBackToInterpreter},   // mtfsb1x
    {134, &BytecodeInterpreter::FallBackToInterpreter},  // mtfsfix
    {711, &BytecodeInterpreter::FallBackToInterpreter},   // mtfsfx
}};

constexpr std::array<BytecodeInterpreterOpTemplate, 10> s_table63_2{{
    {18, &BytecodeInterpreter::FallBackToInterpreter},  // fdivx
    {20, &BytecodeInterpreter::FallBackToInterpreter},  // fsubx
    {21, &BytecodeInterpreter::FallBackToInterpreter},  // faddx
    {23, &BytecodeInterpreter::FallBackToInterpreter},     // fselx
    {25, &BytecodeInterpreter::FallBackToInterpreter},  // fmulx
    {26, &BytecodeInterpreter::FallBackToInterpreter},  // frsqrtex
    {28, &BytecodeInterpreter::FallBackToInterpreter},  // fmsubx
    {29, &BytecodeInterpreter::FallBackToInterpreter},  // fmaddx
    {30, &BytecodeInterpreter::FallBackToInterpreter},  // fnmsubx
    {31, &BytecodeInterpreter::FallBackToInterpreter},  // fnmaddx
}};

constexpr std::array<BytecodeInterpreter::Instruction, 64> s_dyna_op_table = []() consteval
{
  std::array<BytecodeInterpreter::Instruction, 64> table{};
  table.fill(&BytecodeInterpreter::FallBackToInterpreter);

  for (auto& tpl : s_primary_table)
  {
    ASSERT(table[tpl.opcode] == &BytecodeInterpreter::FallBackToInterpreter);
    table[tpl.opcode] = tpl.fn;
  }

  return table;
}
();

constexpr std::array<BytecodeInterpreter::Instruction, 1024> s_dyna_op_table4 = []() consteval
{
  std::array<BytecodeInterpreter::Instruction, 1024> table{};
  table.fill(&BytecodeInterpreter::FallBackToInterpreter);

  for (u32 i = 0; i < 32; i++)
  {
    const u32 fill = i << 5;
    for (const auto& tpl : s_table4_2)
    {
      const u32 op = fill + tpl.opcode;
      ASSERT(table[op] == &BytecodeInterpreter::FallBackToInterpreter);
      table[op] = tpl.fn;
    }
  }

  for (u32 i = 0; i < 16; i++)
  {
    const u32 fill = i << 6;
    for (const auto& tpl : s_table4_3)
    {
      const u32 op = fill + tpl.opcode;
      ASSERT(table[op] == &BytecodeInterpreter::FallBackToInterpreter);
      table[op] = tpl.fn;
    }
  }

  for (const auto& tpl : s_table4)
  {
    const u32 op = tpl.opcode;
    ASSERT(table[op] == &BytecodeInterpreter::FallBackToInterpreter);
    table[op] = tpl.fn;
  }

  return table;
}
();

constexpr std::array<BytecodeInterpreter::Instruction, 1024> s_dyna_op_table19 = []() consteval
{
  std::array<BytecodeInterpreter::Instruction, 1024> table{};
  table.fill(&BytecodeInterpreter::FallBackToInterpreter);

  for (const auto& tpl : s_table19)
  {
    ASSERT(table[tpl.opcode] == &BytecodeInterpreter::FallBackToInterpreter);
    table[tpl.opcode] = tpl.fn;
  }

  return table;
}
();

constexpr std::array<BytecodeInterpreter::Instruction, 1024> s_dyna_op_table31 = []() consteval
{
  std::array<BytecodeInterpreter::Instruction, 1024> table{};
  table.fill(&BytecodeInterpreter::FallBackToInterpreter);

  for (const auto& tpl : s_table31)
  {
    ASSERT(table[tpl.opcode] == &BytecodeInterpreter::FallBackToInterpreter);
    table[tpl.opcode] = tpl.fn;
  }

  return table;
}
();

constexpr std::array<BytecodeInterpreter::Instruction, 32> s_dyna_op_table59 = []() consteval
{
  std::array<BytecodeInterpreter::Instruction, 32> table{};
  table.fill(&BytecodeInterpreter::FallBackToInterpreter);

  for (const auto& tpl : s_table59)
  {
    ASSERT(table[tpl.opcode] == &BytecodeInterpreter::FallBackToInterpreter);
    table[tpl.opcode] = tpl.fn;
  }

  return table;
}
();

constexpr std::array<BytecodeInterpreter::Instruction, 1024> s_dyna_op_table63 = []() consteval
{
  std::array<BytecodeInterpreter::Instruction, 1024> table{};
  table.fill(&BytecodeInterpreter::FallBackToInterpreter);

  for (const auto& tpl : s_table63)
  {
    ASSERT(table[tpl.opcode] == &BytecodeInterpreter::FallBackToInterpreter);
    table[tpl.opcode] = tpl.fn;
  }

  for (u32 i = 0; i < 32; i++)
  {
    const u32 fill = i << 5;
    for (const auto& tpl : s_table63_2)
    {
      const u32 op = fill + tpl.opcode;
      ASSERT(table[op] == &BytecodeInterpreter::FallBackToInterpreter);
      table[op] = tpl.fn;
    }
  }

  return table;
}
();

}  // Anonymous namespace

void BytecodeInterpreter::DynaRunTable4(UGeckoInstruction inst)
{
  (this->*s_dyna_op_table4[inst.SUBOP10])(inst);
}

void BytecodeInterpreter::DynaRunTable19(UGeckoInstruction inst)
{
  (this->*s_dyna_op_table19[inst.SUBOP10])(inst);
}

void BytecodeInterpreter::DynaRunTable31(UGeckoInstruction inst)
{
  (this->*s_dyna_op_table31[inst.SUBOP10])(inst);
}

void BytecodeInterpreter::DynaRunTable59(UGeckoInstruction inst)
{
  (this->*s_dyna_op_table59[inst.SUBOP5])(inst);
}

void BytecodeInterpreter::DynaRunTable63(UGeckoInstruction inst)
{
  (this->*s_dyna_op_table63[inst.SUBOP10])(inst);
}

void BytecodeInterpreter::CompileInstruction(PPCAnalyst::CodeOp& op)
{
  (this->*s_dyna_op_table[op.inst.OPCD])(op.inst);

  PPCTables::CountInstructionCompile(op.opinfo, js.compilerPC);
}
