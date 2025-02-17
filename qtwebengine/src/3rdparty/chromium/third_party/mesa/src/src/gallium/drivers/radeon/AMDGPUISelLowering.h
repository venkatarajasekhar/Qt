//===-- AMDGPUISelLowering.h - AMDGPU Lowering Interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the interface defintiion of the TargetLowering class
// that is common to all AMD GPUs.
//
//===----------------------------------------------------------------------===//

#ifndef AMDGPUISELLOWERING_H
#define AMDGPUISELLOWERING_H

#include "llvm/Target/TargetLowering.h"

namespace llvm {

class MachineRegisterInfo;

class AMDGPUTargetLowering : public TargetLowering
{
private:
  SDValue LowerINTRINSIC_WO_CHAIN(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerUDIVREM(SDValue Op, SelectionDAG &DAG) const;

protected:

  /// CreateLiveInRegister - Helper function that adds Reg to the LiveIn list
  /// of the DAG's MachineFunction.  This returns a Register SDNode representing
  /// Reg.
  SDValue CreateLiveInRegister(SelectionDAG &DAG, const TargetRegisterClass *RC,
                                                  unsigned Reg, EVT VT) const;

  bool isHWTrueValue(SDValue Op) const;
  bool isHWFalseValue(SDValue Op) const;

public:
  AMDGPUTargetLowering(TargetMachine &TM);

  virtual SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                             bool isVarArg,
                             const SmallVectorImpl<ISD::InputArg> &Ins,
                             DebugLoc DL, SelectionDAG &DAG,
                             SmallVectorImpl<SDValue> &InVals) const;

  virtual SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                              bool isVarArg,
                              const SmallVectorImpl<ISD::OutputArg> &Outs,
                              const SmallVectorImpl<SDValue> &OutVals,
                              DebugLoc DL, SelectionDAG &DAG) const;

  virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerIntrinsicIABS(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerIntrinsicLRP(SDValue Op, SelectionDAG &DAG) const;
  virtual const char* getTargetNodeName(unsigned Opcode) const;

// Functions defined in AMDILISelLowering.cpp
public:

  /// computeMaskedBitsForTargetNode - Determine which of the bits specified
  /// in Mask are known to be either zero or one and return them in the
  /// KnownZero/KnownOne bitsets.
  virtual void computeMaskedBitsForTargetNode(const SDValue Op,
                                              APInt &KnownZero,
                                              APInt &KnownOne,
                                              const SelectionDAG &DAG,
                                              unsigned Depth = 0) const;

  virtual bool getTgtMemIntrinsic(IntrinsicInfo &Info,
                                  const CallInst &I, unsigned Intrinsic) const;

  /// isFPImmLegal - We want to mark f32/f64 floating point values as legal.
  bool isFPImmLegal(const APFloat &Imm, EVT VT) const;

  /// ShouldShrinkFPConstant - We don't want to shrink f64/f32 constants.
  bool ShouldShrinkFPConstant(EVT VT) const;

private:
  void InitAMDILLowering();
  SDValue LowerSREM(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSREM8(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSREM16(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSREM32(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSREM64(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSDIV(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSDIV24(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSDIV32(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSDIV64(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSIGN_EXTEND_INREG(SDValue Op, SelectionDAG &DAG) const;
  EVT genIntType(uint32_t size = 32, uint32_t numEle = 1) const;
  SDValue LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFP_ROUND(SDValue Op, SelectionDAG &DAG) const;
};

namespace AMDGPUISD
{

enum
{
  // AMDIL ISD Opcodes
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  MAD,         // 32bit Fused Multiply Add instruction
  VBUILD,      // scalar to vector mov instruction
  CALL,        // Function call based on a single integer
  UMUL,        // 32bit unsigned multiplication
  DIV_INF,      // Divide with infinity returned on zero divisor
  RET_FLAG,
  BRANCH_COND,
  // End AMDIL ISD Opcodes
  BITALIGN,
  FRACT,
  FMAX,
  SMAX,
  UMAX,
  FMIN,
  SMIN,
  UMIN,
  URECIP,
  LAST_AMDGPU_ISD_NUMBER
};


} // End namespace AMDGPUISD

namespace SIISD {

enum {
  SI_FIRST = AMDGPUISD::LAST_AMDGPU_ISD_NUMBER,
  VCC_AND,
  VCC_BITCAST
};

} // End namespace SIISD

} // End namespace llvm

#endif // AMDGPUISELLOWERING_H
