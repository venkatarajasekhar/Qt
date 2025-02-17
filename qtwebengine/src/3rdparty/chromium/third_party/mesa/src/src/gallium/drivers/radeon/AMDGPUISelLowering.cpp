//===-- AMDGPUISelLowering.cpp - AMDGPU Common DAG lowering functions -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the parent TargetLowering class for hardware code gen targets.
//
//===----------------------------------------------------------------------===//

#include "AMDGPUISelLowering.h"
#include "AMDILIntrinsicInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

using namespace llvm;

AMDGPUTargetLowering::AMDGPUTargetLowering(TargetMachine &TM) :
  TargetLowering(TM, new TargetLoweringObjectFileELF())
{

  // Initialize target lowering borrowed from AMDIL
  InitAMDILLowering();

  // We need to custom lower some of the intrinsics
  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);

  // Library functions.  These default to Expand, but we have instructions
  // for them.
  setOperationAction(ISD::FCEIL,  MVT::f32, Legal);
  setOperationAction(ISD::FEXP2,  MVT::f32, Legal);
  setOperationAction(ISD::FRINT,  MVT::f32, Legal);

  setOperationAction(ISD::UDIV, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Custom);
  setOperationAction(ISD::UREM, MVT::i32, Expand);
}

//===---------------------------------------------------------------------===//
// TargetLowering Callbacks
//===---------------------------------------------------------------------===//

SDValue AMDGPUTargetLowering::LowerFormalArguments(
                                      SDValue Chain,
                                      CallingConv::ID CallConv,
                                      bool isVarArg,
                                      const SmallVectorImpl<ISD::InputArg> &Ins,
                                      DebugLoc DL, SelectionDAG &DAG,
                                      SmallVectorImpl<SDValue> &InVals) const
{
  // Lowering of arguments happens in R600LowerKernelParameters, so we can
  // ignore the arguments here.
  for (unsigned i = 0, e = Ins.size(); i < e; ++i) {
    InVals.push_back(SDValue());
  }
  return Chain;
}

SDValue AMDGPUTargetLowering::LowerReturn(
                                     SDValue Chain,
                                     CallingConv::ID CallConv,
                                     bool isVarArg,
                                     const SmallVectorImpl<ISD::OutputArg> &Outs,
                                     const SmallVectorImpl<SDValue> &OutVals,
                                     DebugLoc DL, SelectionDAG &DAG) const
{
  return DAG.getNode(AMDGPUISD::RET_FLAG, DL, MVT::Other, Chain);
}

//===---------------------------------------------------------------------===//
// Target specific lowering
//===---------------------------------------------------------------------===//

SDValue AMDGPUTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG)
    const
{
  switch (Op.getOpcode()) {
  default:
    Op.getNode()->dump();
    assert(0 && "Custom lowering code for this"
        "instruction is not implemented yet!");
    break;
  // AMDIL DAG lowering
  case ISD::SDIV: return LowerSDIV(Op, DAG);
  case ISD::SREM: return LowerSREM(Op, DAG);
  case ISD::BUILD_VECTOR: return LowerBUILD_VECTOR(Op, DAG);
  case ISD::SIGN_EXTEND_INREG: return LowerSIGN_EXTEND_INREG(Op, DAG);
  case ISD::BRCOND: return LowerBRCOND(Op, DAG);
  // AMDGPU DAG lowering
  case ISD::INTRINSIC_WO_CHAIN: return LowerINTRINSIC_WO_CHAIN(Op, DAG);
  case ISD::UDIVREM: return LowerUDIVREM(Op, DAG);
  }
  return Op;
}

SDValue AMDGPUTargetLowering::LowerINTRINSIC_WO_CHAIN(SDValue Op,
    SelectionDAG &DAG) const
{
  unsigned IntrinsicID = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();

  switch (IntrinsicID) {
    default: return Op;
    case AMDGPUIntrinsic::AMDIL_abs:
      return LowerIntrinsicIABS(Op, DAG);
    case AMDGPUIntrinsic::AMDIL_exp:
      return DAG.getNode(ISD::FEXP2, DL, VT, Op.getOperand(1));
    case AMDGPUIntrinsic::AMDIL_fabs:
      return DAG.getNode(ISD::FABS, DL, VT, Op.getOperand(1));
    case AMDGPUIntrinsic::AMDGPU_lrp:
      return LowerIntrinsicLRP(Op, DAG);
    case AMDGPUIntrinsic::AMDIL_fraction:
      return DAG.getNode(AMDGPUISD::FRACT, DL, VT, Op.getOperand(1));
    case AMDGPUIntrinsic::AMDIL_mad:
      return DAG.getNode(AMDGPUISD::MAD, DL, VT, Op.getOperand(1),
                              Op.getOperand(2), Op.getOperand(3));
    case AMDGPUIntrinsic::AMDIL_max:
      return DAG.getNode(AMDGPUISD::FMAX, DL, VT, Op.getOperand(1),
                                                  Op.getOperand(2));
    case AMDGPUIntrinsic::AMDGPU_imax:
      return DAG.getNode(AMDGPUISD::SMAX, DL, VT, Op.getOperand(1),
                                                  Op.getOperand(2));
    case AMDGPUIntrinsic::AMDGPU_umax:
      return DAG.getNode(AMDGPUISD::UMAX, DL, VT, Op.getOperand(1),
                                                  Op.getOperand(2));
    case AMDGPUIntrinsic::AMDIL_min:
      return DAG.getNode(AMDGPUISD::FMIN, DL, VT, Op.getOperand(1),
                                                  Op.getOperand(2));
    case AMDGPUIntrinsic::AMDGPU_imin:
      return DAG.getNode(AMDGPUISD::SMIN, DL, VT, Op.getOperand(1),
                                                  Op.getOperand(2));
    case AMDGPUIntrinsic::AMDGPU_umin:
      return DAG.getNode(AMDGPUISD::UMIN, DL, VT, Op.getOperand(1),
                                                  Op.getOperand(2));
    case AMDGPUIntrinsic::AMDIL_round_nearest:
      return DAG.getNode(ISD::FRINT, DL, VT, Op.getOperand(1));
    case AMDGPUIntrinsic::AMDIL_round_posinf:
      return DAG.getNode(ISD::FCEIL, DL, VT, Op.getOperand(1));
  }
}

///IABS(a) = SMAX(sub(0, a), a)
SDValue AMDGPUTargetLowering::LowerIntrinsicIABS(SDValue Op,
    SelectionDAG &DAG) const
{

  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();
  SDValue Neg = DAG.getNode(ISD::SUB, DL, VT, DAG.getConstant(0, VT),
                                              Op.getOperand(1));

  return DAG.getNode(AMDGPUISD::SMAX, DL, VT, Neg, Op.getOperand(1));
}

/// Linear Interpolation
/// LRP(a, b, c) = muladd(a,  b, (1 - a) * c)
SDValue AMDGPUTargetLowering::LowerIntrinsicLRP(SDValue Op,
    SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();
  SDValue OneSubA = DAG.getNode(ISD::FSUB, DL, VT,
                                DAG.getConstantFP(1.0f, MVT::f32),
                                Op.getOperand(1));
  SDValue OneSubAC = DAG.getNode(ISD::FMUL, DL, VT, OneSubA,
                                                    Op.getOperand(3));
  return DAG.getNode(AMDGPUISD::MAD, DL, VT, Op.getOperand(1),
                                               Op.getOperand(2),
                                               OneSubAC);
}



SDValue AMDGPUTargetLowering::LowerUDIVREM(SDValue Op,
    SelectionDAG &DAG) const
{
  DebugLoc DL = Op.getDebugLoc();
  EVT VT = Op.getValueType();

  SDValue Num = Op.getOperand(0);
  SDValue Den = Op.getOperand(1);

  SmallVector<SDValue, 8> Results;

  // RCP =  URECIP(Den) = 2^32 / Den + e
  // e is rounding error.
  SDValue RCP = DAG.getNode(AMDGPUISD::URECIP, DL, VT, Den);

  // RCP_LO = umulo(RCP, Den) */
  SDValue RCP_LO = DAG.getNode(ISD::UMULO, DL, VT, RCP, Den);

  // RCP_HI = mulhu (RCP, Den) */
  SDValue RCP_HI = DAG.getNode(ISD::MULHU, DL, VT, RCP, Den);

  // NEG_RCP_LO = -RCP_LO
  SDValue NEG_RCP_LO = DAG.getNode(ISD::SUB, DL, VT, DAG.getConstant(0, VT),
                                                     RCP_LO);

  // ABS_RCP_LO = (RCP_HI == 0 ? NEG_RCP_LO : RCP_LO)
  SDValue ABS_RCP_LO = DAG.getSelectCC(DL, RCP_HI, DAG.getConstant(0, VT),
                                           NEG_RCP_LO, RCP_LO,
                                           ISD::SETEQ);
  // Calculate the rounding error from the URECIP instruction
  // E = mulhu(ABS_RCP_LO, RCP)
  SDValue E = DAG.getNode(ISD::MULHU, DL, VT, ABS_RCP_LO, RCP);

  // RCP_A_E = RCP + E
  SDValue RCP_A_E = DAG.getNode(ISD::ADD, DL, VT, RCP, E);

  // RCP_S_E = RCP - E
  SDValue RCP_S_E = DAG.getNode(ISD::SUB, DL, VT, RCP, E);

  // Tmp0 = (RCP_HI == 0 ? RCP_A_E : RCP_SUB_E)
  SDValue Tmp0 = DAG.getSelectCC(DL, RCP_HI, DAG.getConstant(0, VT),
                                     RCP_A_E, RCP_S_E,
                                     ISD::SETEQ);
  // Quotient = mulhu(Tmp0, Num)
  SDValue Quotient = DAG.getNode(ISD::MULHU, DL, VT, Tmp0, Num);

  // Num_S_Remainder = Quotient * Den
  SDValue Num_S_Remainder = DAG.getNode(ISD::UMULO, DL, VT, Quotient, Den);

  // Remainder = Num - Num_S_Remainder
  SDValue Remainder = DAG.getNode(ISD::SUB, DL, VT, Num, Num_S_Remainder);

  // Remainder_GE_Den = (Remainder >= Den ? -1 : 0)
  SDValue Remainder_GE_Den = DAG.getSelectCC(DL, Remainder, Den,
                                                 DAG.getConstant(-1, VT),
                                                 DAG.getConstant(0, VT),
                                                 ISD::SETGE);
  // Remainder_GE_Zero = (Remainder >= 0 ? -1 : 0)
  SDValue Remainder_GE_Zero = DAG.getSelectCC(DL, Remainder,
                                                  DAG.getConstant(0, VT),
                                                  DAG.getConstant(-1, VT),
                                                  DAG.getConstant(0, VT),
                                                  ISD::SETGE);
  // Tmp1 = Remainder_GE_Den & Remainder_GE_Zero
  SDValue Tmp1 = DAG.getNode(ISD::AND, DL, VT, Remainder_GE_Den,
                                               Remainder_GE_Zero);

  // Calculate Division result:

  // Quotient_A_One = Quotient + 1
  SDValue Quotient_A_One = DAG.getNode(ISD::ADD, DL, VT, Quotient,
                                                         DAG.getConstant(1, VT));

  // Quotient_S_One = Quotient - 1
  SDValue Quotient_S_One = DAG.getNode(ISD::SUB, DL, VT, Quotient,
                                                         DAG.getConstant(1, VT));

  // Div = (Tmp1 == 0 ? Quotient : Quotient_A_One)
  SDValue Div = DAG.getSelectCC(DL, Tmp1, DAG.getConstant(0, VT),
                                     Quotient, Quotient_A_One, ISD::SETEQ);

  // Div = (Remainder_GE_Zero == 0 ? Quotient_S_One : Div)
  Div = DAG.getSelectCC(DL, Remainder_GE_Zero, DAG.getConstant(0, VT),
                            Quotient_S_One, Div, ISD::SETEQ);

  // Calculate Rem result:

  // Remainder_S_Den = Remainder - Den
  SDValue Remainder_S_Den = DAG.getNode(ISD::SUB, DL, VT, Remainder, Den);

  // Remainder_A_Den = Remainder + Den
  SDValue Remainder_A_Den = DAG.getNode(ISD::ADD, DL, VT, Remainder, Den);

  // Rem = (Tmp1 == 0 ? Remainder : Remainder_S_Den)
  SDValue Rem = DAG.getSelectCC(DL, Tmp1, DAG.getConstant(0, VT),
                                    Remainder, Remainder_S_Den, ISD::SETEQ);

  // Rem = (Remainder_GE_Zero == 0 ? Remainder_A_Den : Rem)
  Rem = DAG.getSelectCC(DL, Remainder_GE_Zero, DAG.getConstant(0, VT),
                            Remainder_A_Den, Rem, ISD::SETEQ);

  DAG.ReplaceAllUsesWith(Op.getValue(0).getNode(), &Div);
  DAG.ReplaceAllUsesWith(Op.getValue(1).getNode(), &Rem);

  return Op;
}

//===----------------------------------------------------------------------===//
// Helper functions
//===----------------------------------------------------------------------===//

bool AMDGPUTargetLowering::isHWTrueValue(SDValue Op) const
{
  if (ConstantFPSDNode * CFP = dyn_cast<ConstantFPSDNode>(Op)) {
    return CFP->isExactlyValue(1.0);
  }
  if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
    return C->isAllOnesValue();
  }
  return false;
}

bool AMDGPUTargetLowering::isHWFalseValue(SDValue Op) const
{
  if (ConstantFPSDNode * CFP = dyn_cast<ConstantFPSDNode>(Op)) {
    return CFP->getValueAPF().isZero();
  }
  if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op)) {
    return C->isNullValue();
  }
  return false;
}

SDValue AMDGPUTargetLowering::CreateLiveInRegister(SelectionDAG &DAG,
                                                  const TargetRegisterClass *RC,
                                                   unsigned Reg, EVT VT) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  unsigned VirtualRegister;
  if (!MRI.isLiveIn(Reg)) {
    VirtualRegister = MRI.createVirtualRegister(RC);
    MRI.addLiveIn(Reg, VirtualRegister);
  } else {
    VirtualRegister = MRI.getLiveInVirtReg(Reg);
  }
  return DAG.getRegister(VirtualRegister, VT);
}

#define NODE_NAME_CASE(node) case AMDGPUISD::node: return #node;

const char* AMDGPUTargetLowering::getTargetNodeName(unsigned Opcode) const
{
  switch (Opcode) {
  default: return 0;
  // AMDIL DAG nodes
  NODE_NAME_CASE(MAD);
  NODE_NAME_CASE(CALL);
  NODE_NAME_CASE(UMUL);
  NODE_NAME_CASE(DIV_INF);
  NODE_NAME_CASE(VBUILD);
  NODE_NAME_CASE(RET_FLAG);
  NODE_NAME_CASE(BRANCH_COND);

  // AMDGPU DAG nodes
  NODE_NAME_CASE(FRACT)
  NODE_NAME_CASE(FMAX)
  NODE_NAME_CASE(SMAX)
  NODE_NAME_CASE(UMAX)
  NODE_NAME_CASE(FMIN)
  NODE_NAME_CASE(SMIN)
  NODE_NAME_CASE(UMIN)
  NODE_NAME_CASE(URECIP)
  }
}
