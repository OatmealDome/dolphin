# ISA

8 GPR (R0 to R7)
PC

## Register Manipulation

LoadImm32 rD, imm32
Mov rD, rA

## ALU

Add rD, rA, rB
Sub rD, rA, rB

And rD, rA, rB
Or rD, rA, rB
Xor rD, rA, rB
Not Rd, rA

ShiftLeft rD, rA, rB
ShiftRight rD, rA, rB

## Memory

LoadSafe32 rD, rA
LoadUnchecked32 rD, imm
LoadMmio32 rD, callback

## Host/Guest State Transfer

host = IR
guest = PPC

TransferRegHostToGuest rG, rH
TransferRegGuestToHost rH, rG
TransferRegImm32ToGuest rG, imm32

## Cached Interpreter

StartProfiledBlock
EndBlock
WritePC
Interpret
CheckException
HLEFunction
WriteBrokenBlockNPC
CheckHalt
CheckIdle

