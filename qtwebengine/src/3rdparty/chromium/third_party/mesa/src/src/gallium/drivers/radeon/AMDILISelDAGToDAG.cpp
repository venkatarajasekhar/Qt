//===-- AMDILISelDAGToDAG.cpp - A dag to dag inst selector for AMDIL ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the AMDIL target.
//
//===----------------------------------------------------------------------===//
#include "AMDGPUInstrInfo.h"
#include "AMDGPUISelLowering.h" // For AMDGPUISD
#include "AMDGPURegisterInfo.h"
#include "AMDILDevices.h"
#include "AMDILUtilityFunctions.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Compiler.h"
#include <list>
#include <queue>

using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// AMDGPUDAGToDAGISel - AMDGPU specific code to select AMDGPU machine instructions
// //for SelectionDAG operations.
//
namespace {
class AMDGPUDAGToDAGISel : public SelectionDAGISel {
  // Subtarget - Keep a pointer to the AMDGPU Subtarget around so that we can
  // make the right decision when generating code for different targets.
  const AMDGPUSubtarget &Subtarget;
public:
  AMDGPUDAGToDAGISel(TargetMachine &TM);
  virtual ~AMDGPUDAGToDAGISel();

  SDNode *Select(SDNode *N);
  virtual const char *getPassName() const;

private:
  inline SDValue getSmallIPtrImm(unsigned Imm);

  // Complex pattern selectors
  bool SelectADDRParam(SDValue Addr, SDValue& R1, SDValue& R2);
  bool SelectADDR(SDValue N, SDValue &R1, SDValue &R2);
  bool SelectADDR64(SDValue N, SDValue &R1, SDValue &R2);

  static bool checkType(const Value *ptr, unsigned int addrspace);
  static const Value *getBasePointerValue(const Value *V);

  static bool isGlobalStore(const StoreSDNode *N);
  static bool isPrivateStore(const StoreSDNode *N);
  static bool isLocalStore(const StoreSDNode *N);
  static bool isRegionStore(const StoreSDNode *N);

  static bool isCPLoad(const LoadSDNode *N);
  static bool isConstantLoad(const LoadSDNode *N, int cbID);
  static bool isGlobalLoad(const LoadSDNode *N);
  static bool isPrivateLoad(const LoadSDNode *N);
  static bool isLocalLoad(const LoadSDNode *N);
  static bool isRegionLoad(const LoadSDNode *N);

  bool SelectADDR8BitOffset(SDValue Addr, SDValue& Base, SDValue& Offset);
  bool SelectADDRReg(SDValue Addr, SDValue& Base, SDValue& Offset);
  bool SelectADDRVTX_READ(SDValue Addr, SDValue &Base, SDValue &Offset);

  // Include the pieces autogenerated from the target description.
#include "AMDGPUGenDAGISel.inc"
};
}  // end anonymous namespace

// createAMDGPUISelDag - This pass converts a legalized DAG into a AMDGPU-specific
// DAG, ready for instruction scheduling.
//
FunctionPass *llvm::createAMDGPUISelDag(TargetMachine &TM
                                       ) {
  return new AMDGPUDAGToDAGISel(TM);
}

AMDGPUDAGToDAGISel::AMDGPUDAGToDAGISel(TargetMachine &TM
                                     )
  : SelectionDAGISel(TM), Subtarget(TM.getSubtarget<AMDGPUSubtarget>())
{
}

AMDGPUDAGToDAGISel::~AMDGPUDAGToDAGISel() {
}

SDValue AMDGPUDAGToDAGISel::getSmallIPtrImm(unsigned int Imm) {
  return CurDAG->getTargetConstant(Imm, MVT::i32);
}

bool AMDGPUDAGToDAGISel::SelectADDRParam(
    SDValue Addr, SDValue& R1, SDValue& R2) {

  if (Addr.getOpcode() == ISD::FrameIndex) {
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
      R1 = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
      R2 = CurDAG->getTargetConstant(0, MVT::i32);
    } else {
      R1 = Addr;
      R2 = CurDAG->getTargetConstant(0, MVT::i32);
    }
  } else if (Addr.getOpcode() == ISD::ADD) {
    R1 = Addr.getOperand(0);
    R2 = Addr.getOperand(1);
  } else {
    R1 = Addr;
    R2 = CurDAG->getTargetConstant(0, MVT::i32);
  }
  return true;
}

bool AMDGPUDAGToDAGISel::SelectADDR(SDValue Addr, SDValue& R1, SDValue& R2) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }
  return SelectADDRParam(Addr, R1, R2);
}


bool AMDGPUDAGToDAGISel::SelectADDR64(SDValue Addr, SDValue& R1, SDValue& R2) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }

  if (Addr.getOpcode() == ISD::FrameIndex) {
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
      R1 = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
      R2 = CurDAG->getTargetConstant(0, MVT::i64);
    } else {
      R1 = Addr;
      R2 = CurDAG->getTargetConstant(0, MVT::i64);
    }
  } else if (Addr.getOpcode() == ISD::ADD) {
    R1 = Addr.getOperand(0);
    R2 = Addr.getOperand(1);
  } else {
    R1 = Addr;
    R2 = CurDAG->getTargetConstant(0, MVT::i64);
  }
  return true;
}

SDNode *AMDGPUDAGToDAGISel::Select(SDNode *N) {
  unsigned int Opc = N->getOpcode();
  if (N->isMachineOpcode()) {
    return NULL;   // Already selected.
  }
  switch (Opc) {
  default: break;
  case ISD::FrameIndex:
    {
      if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(N)) {
        unsigned int FI = FIN->getIndex();
        EVT OpVT = N->getValueType(0);
        unsigned int NewOpc = AMDGPU::COPY;
        SDValue TFI = CurDAG->getTargetFrameIndex(FI, MVT::i32);
        return CurDAG->SelectNodeTo(N, NewOpc, OpVT, TFI);
      }
    }
    break;
  }
  return SelectCode(N);
}

bool AMDGPUDAGToDAGISel::checkType(const Value *ptr, unsigned int addrspace) {
  if (!ptr) {
    return false;
  }
  Type *ptrType = ptr->getType();
  return dyn_cast<PointerType>(ptrType)->getAddressSpace() == addrspace;
}

const Value * AMDGPUDAGToDAGISel::getBasePointerValue(const Value *V)
{
  if (!V) {
    return NULL;
  }
  const Value *ret = NULL;
  ValueMap<const Value *, bool> ValueBitMap;
  std::queue<const Value *, std::list<const Value *> > ValueQueue;
  ValueQueue.push(V);
  while (!ValueQueue.empty()) {
    V = ValueQueue.front();
    if (ValueBitMap.find(V) == ValueBitMap.end()) {
      ValueBitMap[V] = true;
      if (dyn_cast<Argument>(V) && dyn_cast<PointerType>(V->getType())) {
        ret = V;
        break;
      } else if (dyn_cast<GlobalVariable>(V)) {
        ret = V;
        break;
      } else if (dyn_cast<Constant>(V)) {
        const ConstantExpr *CE = dyn_cast<ConstantExpr>(V);
        if (CE) {
          ValueQueue.push(CE->getOperand(0));
        }
      } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(V)) {
        ret = AI;
        break;
      } else if (const Instruction *I = dyn_cast<Instruction>(V)) {
        uint32_t numOps = I->getNumOperands();
        for (uint32_t x = 0; x < numOps; ++x) {
          ValueQueue.push(I->getOperand(x));
        }
      } else {
        // assert(0 && "Found a Value that we didn't know how to handle!");
      }
    }
    ValueQueue.pop();
  }
  return ret;
}

bool AMDGPUDAGToDAGISel::isGlobalStore(const StoreSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isPrivateStore(const StoreSDNode *N) {
  return (!checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS)
          && !checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS)
          && !checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS));
}

bool AMDGPUDAGToDAGISel::isLocalStore(const StoreSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isRegionStore(const StoreSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isConstantLoad(const LoadSDNode *N, int cbID) {
  if (checkType(N->getSrcValue(), AMDGPUAS::CONSTANT_ADDRESS)) {
    return true;
  }
  MachineMemOperand *MMO = N->getMemOperand();
  const Value *V = MMO->getValue();
  const Value *BV = getBasePointerValue(V);
  if (MMO
      && MMO->getValue()
      && ((V && dyn_cast<GlobalValue>(V))
          || (BV && dyn_cast<GlobalValue>(
                        getBasePointerValue(MMO->getValue()))))) {
    return checkType(N->getSrcValue(), AMDGPUAS::PRIVATE_ADDRESS);
  } else {
    return false;
  }
}

bool AMDGPUDAGToDAGISel::isGlobalLoad(const LoadSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isLocalLoad(const  LoadSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isRegionLoad(const  LoadSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isCPLoad(const LoadSDNode *N) {
  MachineMemOperand *MMO = N->getMemOperand();
  if (checkType(N->getSrcValue(), AMDGPUAS::PRIVATE_ADDRESS)) {
    if (MMO) {
      const Value *V = MMO->getValue();
      const PseudoSourceValue *PSV = dyn_cast<PseudoSourceValue>(V);
      if (PSV && PSV == PseudoSourceValue::getConstantPool()) {
        return true;
      }
    }
  }
  return false;
}

bool AMDGPUDAGToDAGISel::isPrivateLoad(const LoadSDNode *N) {
  if (checkType(N->getSrcValue(), AMDGPUAS::PRIVATE_ADDRESS)) {
    // Check to make sure we are not a constant pool load or a constant load
    // that is marked as a private load
    if (isCPLoad(N) || isConstantLoad(N, -1)) {
      return false;
    }
  }
  if (!checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::CONSTANT_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::PARAM_D_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::PARAM_I_ADDRESS))
  {
    return true;
  }
  return false;
}

const char *AMDGPUDAGToDAGISel::getPassName() const {
  return "AMDGPU DAG->DAG Pattern Instruction Selection";
}

#ifdef DEBUGTMP
#undef INT64_C
#endif
#undef DEBUGTMP

///==== AMDGPU Functions ====///

bool AMDGPUDAGToDAGISel::SelectADDR8BitOffset(SDValue Addr, SDValue& Base,
                                             SDValue& Offset) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }


  if (Addr.getOpcode() == ISD::ADD) {
    bool Match = false;

    // Find the base ptr and the offset
    for (unsigned i = 0; i < Addr.getNumOperands(); i++) {
      SDValue Arg = Addr.getOperand(i);
      ConstantSDNode * OffsetNode = dyn_cast<ConstantSDNode>(Arg);
      // This arg isn't a constant so it must be the base PTR.
      if (!OffsetNode) {
        Base = Addr.getOperand(i);
        continue;
      }
      // Check if the constant argument fits in 8-bits.  The offset is in bytes
      // so we need to convert it to dwords.
      if (isInt<8>(OffsetNode->getZExtValue() >> 2)) {
        Match = true;
        Offset = CurDAG->getTargetConstant(OffsetNode->getZExtValue() >> 2,
                                           MVT::i32);
      }
    }
    return Match;
  }

  // Default case, no offset
  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool AMDGPUDAGToDAGISel::SelectADDRVTX_READ(SDValue Addr, SDValue &Base,
                                           SDValue &Offset)
{
  ConstantSDNode * IMMOffset;

  if (Addr.getOpcode() == ISD::ADD
      && (IMMOffset = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))
      && isInt<16>(IMMOffset->getZExtValue())) {

      Base = Addr.getOperand(0);
      Offset = CurDAG->getTargetConstant(IMMOffset->getZExtValue(), MVT::i32);
      return true;
  // If the pointer address is constant, we can move it to the offset field.
  } else if ((IMMOffset = dyn_cast<ConstantSDNode>(Addr))
             && isInt<16>(IMMOffset->getZExtValue())) {
    Base = CurDAG->getCopyFromReg(CurDAG->getEntryNode(),
                                  CurDAG->getEntryNode().getDebugLoc(),
                                  AMDGPU::ZERO, MVT::i32);
    Offset = CurDAG->getTargetConstant(IMMOffset->getZExtValue(), MVT::i32);
    return true;
  }

  // Default case, no offset
  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool AMDGPUDAGToDAGISel::SelectADDRReg(SDValue Addr, SDValue& Base,
                                      SDValue& Offset) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress  ||
      Addr.getOpcode() != ISD::ADD) {
    return false;
  }

  Base = Addr.getOperand(0);
  Offset = Addr.getOperand(1);

  return true;
}
