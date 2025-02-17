// Copyright (c) 1994-2006 Sun Microsystems Inc.
// All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// - Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// - Redistribution in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution.
//
// - Neither the name of Sun Microsystems or the names of contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

// The original source code covered by the above license above has been modified
// significantly by Google Inc.
// Copyright 2012 the V8 project authors. All rights reserved.

#ifndef V8_ARM_ASSEMBLER_ARM_INL_H_
#define V8_ARM_ASSEMBLER_ARM_INL_H_

#include "src/arm/assembler-arm.h"

#include "src/cpu.h"
#include "src/debug.h"


namespace v8 {
namespace internal {


bool CpuFeatures::SupportsCrankshaft() { return IsSupported(VFP3); }


int Register::NumAllocatableRegisters() {
  return kMaxNumAllocatableRegisters;
}


int DwVfpRegister::NumRegisters() {
  return CpuFeatures::IsSupported(VFP32DREGS) ? 32 : 16;
}


int DwVfpRegister::NumReservedRegisters() {
  return kNumReservedRegisters;
}


int DwVfpRegister::NumAllocatableRegisters() {
  return NumRegisters() - kNumReservedRegisters;
}


int DwVfpRegister::ToAllocationIndex(DwVfpRegister reg) {
  ASSERT(!reg.is(kDoubleRegZero));
  ASSERT(!reg.is(kScratchDoubleReg));
  if (reg.code() > kDoubleRegZero.code()) {
    return reg.code() - kNumReservedRegisters;
  }
  return reg.code();
}


DwVfpRegister DwVfpRegister::FromAllocationIndex(int index) {
  ASSERT(index >= 0 && index < NumAllocatableRegisters());
  ASSERT(kScratchDoubleReg.code() - kDoubleRegZero.code() ==
         kNumReservedRegisters - 1);
  if (index >= kDoubleRegZero.code()) {
    return from_code(index + kNumReservedRegisters);
  }
  return from_code(index);
}


void RelocInfo::apply(intptr_t delta, ICacheFlushMode icache_flush_mode) {
  if (RelocInfo::IsInternalReference(rmode_)) {
    // absolute code pointer inside code object moves with the code object.
    int32_t* p = reinterpret_cast<int32_t*>(pc_);
    *p += delta;  // relocate entry
  }
  // We do not use pc relative addressing on ARM, so there is
  // nothing else to do.
}


Address RelocInfo::target_address() {
  ASSERT(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_));
  return Assembler::target_address_at(pc_, host_);
}


Address RelocInfo::target_address_address() {
  ASSERT(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_)
                              || rmode_ == EMBEDDED_OBJECT
                              || rmode_ == EXTERNAL_REFERENCE);
  if (FLAG_enable_ool_constant_pool ||
      Assembler::IsMovW(Memory::int32_at(pc_))) {
    // We return the PC for ool constant pool since this function is used by the
    // serializerer and expects the address to reside within the code object.
    return reinterpret_cast<Address>(pc_);
  } else {
    ASSERT(Assembler::IsLdrPcImmediateOffset(Memory::int32_at(pc_)));
    return Assembler::target_pointer_address_at(pc_);
  }
}


Address RelocInfo::constant_pool_entry_address() {
  ASSERT(IsInConstantPool());
  if (FLAG_enable_ool_constant_pool) {
    ASSERT(Assembler::IsLdrPpImmediateOffset(Memory::int32_at(pc_)));
    return Assembler::target_constant_pool_address_at(pc_,
                                                      host_->constant_pool());
  } else {
    ASSERT(Assembler::IsLdrPcImmediateOffset(Memory::int32_at(pc_)));
    return Assembler::target_pointer_address_at(pc_);
  }
}


int RelocInfo::target_address_size() {
  return kPointerSize;
}


void RelocInfo::set_target_address(Address target,
                                   WriteBarrierMode write_barrier_mode,
                                   ICacheFlushMode icache_flush_mode) {
  ASSERT(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_));
  Assembler::set_target_address_at(pc_, host_, target, icache_flush_mode);
  if (write_barrier_mode == UPDATE_WRITE_BARRIER &&
      host() != NULL && IsCodeTarget(rmode_)) {
    Object* target_code = Code::GetCodeFromTargetAddress(target);
    host()->GetHeap()->incremental_marking()->RecordWriteIntoCode(
        host(), this, HeapObject::cast(target_code));
  }
}


Object* RelocInfo::target_object() {
  ASSERT(IsCodeTarget(rmode_) || rmode_ == EMBEDDED_OBJECT);
  return reinterpret_cast<Object*>(Assembler::target_address_at(pc_, host_));
}


Handle<Object> RelocInfo::target_object_handle(Assembler* origin) {
  ASSERT(IsCodeTarget(rmode_) || rmode_ == EMBEDDED_OBJECT);
  return Handle<Object>(reinterpret_cast<Object**>(
      Assembler::target_address_at(pc_, host_)));
}


void RelocInfo::set_target_object(Object* target,
                                  WriteBarrierMode write_barrier_mode,
                                  ICacheFlushMode icache_flush_mode) {
  ASSERT(IsCodeTarget(rmode_) || rmode_ == EMBEDDED_OBJECT);
  ASSERT(!target->IsConsString());
  Assembler::set_target_address_at(pc_, host_,
                                   reinterpret_cast<Address>(target),
                                   icache_flush_mode);
  if (write_barrier_mode == UPDATE_WRITE_BARRIER &&
      host() != NULL &&
      target->IsHeapObject()) {
    host()->GetHeap()->incremental_marking()->RecordWrite(
        host(), &Memory::Object_at(pc_), HeapObject::cast(target));
  }
}


Address RelocInfo::target_reference() {
  ASSERT(rmode_ == EXTERNAL_REFERENCE);
  return Assembler::target_address_at(pc_, host_);
}


Address RelocInfo::target_runtime_entry(Assembler* origin) {
  ASSERT(IsRuntimeEntry(rmode_));
  return target_address();
}


void RelocInfo::set_target_runtime_entry(Address target,
                                         WriteBarrierMode write_barrier_mode,
                                         ICacheFlushMode icache_flush_mode) {
  ASSERT(IsRuntimeEntry(rmode_));
  if (target_address() != target)
    set_target_address(target, write_barrier_mode, icache_flush_mode);
}


Handle<Cell> RelocInfo::target_cell_handle() {
  ASSERT(rmode_ == RelocInfo::CELL);
  Address address = Memory::Address_at(pc_);
  return Handle<Cell>(reinterpret_cast<Cell**>(address));
}


Cell* RelocInfo::target_cell() {
  ASSERT(rmode_ == RelocInfo::CELL);
  return Cell::FromValueAddress(Memory::Address_at(pc_));
}


void RelocInfo::set_target_cell(Cell* cell,
                                WriteBarrierMode write_barrier_mode,
                                ICacheFlushMode icache_flush_mode) {
  ASSERT(rmode_ == RelocInfo::CELL);
  Address address = cell->address() + Cell::kValueOffset;
  Memory::Address_at(pc_) = address;
  if (write_barrier_mode == UPDATE_WRITE_BARRIER && host() != NULL) {
    // TODO(1550) We are passing NULL as a slot because cell can never be on
    // evacuation candidate.
    host()->GetHeap()->incremental_marking()->RecordWrite(
        host(), NULL, cell);
  }
}


static const int kNoCodeAgeSequenceLength = 3 * Assembler::kInstrSize;


Handle<Object> RelocInfo::code_age_stub_handle(Assembler* origin) {
  UNREACHABLE();  // This should never be reached on Arm.
  return Handle<Object>();
}


Code* RelocInfo::code_age_stub() {
  ASSERT(rmode_ == RelocInfo::CODE_AGE_SEQUENCE);
  return Code::GetCodeFromTargetAddress(
      Memory::Address_at(pc_ +
                         (kNoCodeAgeSequenceLength - Assembler::kInstrSize)));
}


void RelocInfo::set_code_age_stub(Code* stub,
                                  ICacheFlushMode icache_flush_mode) {
  ASSERT(rmode_ == RelocInfo::CODE_AGE_SEQUENCE);
  Memory::Address_at(pc_ +
                     (kNoCodeAgeSequenceLength - Assembler::kInstrSize)) =
      stub->instruction_start();
}


Address RelocInfo::call_address() {
  // The 2 instructions offset assumes patched debug break slot or return
  // sequence.
  ASSERT((IsJSReturn(rmode()) && IsPatchedReturnSequence()) ||
         (IsDebugBreakSlot(rmode()) && IsPatchedDebugBreakSlotSequence()));
  return Memory::Address_at(pc_ + 2 * Assembler::kInstrSize);
}


void RelocInfo::set_call_address(Address target) {
  ASSERT((IsJSReturn(rmode()) && IsPatchedReturnSequence()) ||
         (IsDebugBreakSlot(rmode()) && IsPatchedDebugBreakSlotSequence()));
  Memory::Address_at(pc_ + 2 * Assembler::kInstrSize) = target;
  if (host() != NULL) {
    Object* target_code = Code::GetCodeFromTargetAddress(target);
    host()->GetHeap()->incremental_marking()->RecordWriteIntoCode(
        host(), this, HeapObject::cast(target_code));
  }
}


Object* RelocInfo::call_object() {
  return *call_object_address();
}


void RelocInfo::set_call_object(Object* target) {
  *call_object_address() = target;
}


Object** RelocInfo::call_object_address() {
  ASSERT((IsJSReturn(rmode()) && IsPatchedReturnSequence()) ||
         (IsDebugBreakSlot(rmode()) && IsPatchedDebugBreakSlotSequence()));
  return reinterpret_cast<Object**>(pc_ + 2 * Assembler::kInstrSize);
}


void RelocInfo::WipeOut() {
  ASSERT(IsEmbeddedObject(rmode_) ||
         IsCodeTarget(rmode_) ||
         IsRuntimeEntry(rmode_) ||
         IsExternalReference(rmode_));
  Assembler::set_target_address_at(pc_, host_, NULL);
}


bool RelocInfo::IsPatchedReturnSequence() {
  Instr current_instr = Assembler::instr_at(pc_);
  Instr next_instr = Assembler::instr_at(pc_ + Assembler::kInstrSize);
  // A patched return sequence is:
  //  ldr ip, [pc, #0]
  //  blx ip
  return ((current_instr & kLdrPCMask) == kLdrPCPattern)
          && ((next_instr & kBlxRegMask) == kBlxRegPattern);
}


bool RelocInfo::IsPatchedDebugBreakSlotSequence() {
  Instr current_instr = Assembler::instr_at(pc_);
  return !Assembler::IsNop(current_instr, Assembler::DEBUG_BREAK_NOP);
}


void RelocInfo::Visit(Isolate* isolate, ObjectVisitor* visitor) {
  RelocInfo::Mode mode = rmode();
  if (mode == RelocInfo::EMBEDDED_OBJECT) {
    visitor->VisitEmbeddedPointer(this);
  } else if (RelocInfo::IsCodeTarget(mode)) {
    visitor->VisitCodeTarget(this);
  } else if (mode == RelocInfo::CELL) {
    visitor->VisitCell(this);
  } else if (mode == RelocInfo::EXTERNAL_REFERENCE) {
    visitor->VisitExternalReference(this);
  } else if (RelocInfo::IsCodeAgeSequence(mode)) {
    visitor->VisitCodeAgeSequence(this);
  } else if (((RelocInfo::IsJSReturn(mode) &&
              IsPatchedReturnSequence()) ||
             (RelocInfo::IsDebugBreakSlot(mode) &&
              IsPatchedDebugBreakSlotSequence())) &&
             isolate->debug()->has_break_points()) {
    visitor->VisitDebugTarget(this);
  } else if (RelocInfo::IsRuntimeEntry(mode)) {
    visitor->VisitRuntimeEntry(this);
  }
}


template<typename StaticVisitor>
void RelocInfo::Visit(Heap* heap) {
  RelocInfo::Mode mode = rmode();
  if (mode == RelocInfo::EMBEDDED_OBJECT) {
    StaticVisitor::VisitEmbeddedPointer(heap, this);
  } else if (RelocInfo::IsCodeTarget(mode)) {
    StaticVisitor::VisitCodeTarget(heap, this);
  } else if (mode == RelocInfo::CELL) {
    StaticVisitor::VisitCell(heap, this);
  } else if (mode == RelocInfo::EXTERNAL_REFERENCE) {
    StaticVisitor::VisitExternalReference(this);
  } else if (RelocInfo::IsCodeAgeSequence(mode)) {
    StaticVisitor::VisitCodeAgeSequence(heap, this);
  } else if (heap->isolate()->debug()->has_break_points() &&
             ((RelocInfo::IsJSReturn(mode) &&
              IsPatchedReturnSequence()) ||
             (RelocInfo::IsDebugBreakSlot(mode) &&
              IsPatchedDebugBreakSlotSequence()))) {
    StaticVisitor::VisitDebugTarget(heap, this);
  } else if (RelocInfo::IsRuntimeEntry(mode)) {
    StaticVisitor::VisitRuntimeEntry(this);
  }
}


Operand::Operand(int32_t immediate, RelocInfo::Mode rmode)  {
  rm_ = no_reg;
  imm32_ = immediate;
  rmode_ = rmode;
}


Operand::Operand(const ExternalReference& f)  {
  rm_ = no_reg;
  imm32_ = reinterpret_cast<int32_t>(f.address());
  rmode_ = RelocInfo::EXTERNAL_REFERENCE;
}


Operand::Operand(Smi* value) {
  rm_ = no_reg;
  imm32_ =  reinterpret_cast<intptr_t>(value);
  rmode_ = RelocInfo::NONE32;
}


Operand::Operand(Register rm) {
  rm_ = rm;
  rs_ = no_reg;
  shift_op_ = LSL;
  shift_imm_ = 0;
}


bool Operand::is_reg() const {
  return rm_.is_valid() &&
         rs_.is(no_reg) &&
         shift_op_ == LSL &&
         shift_imm_ == 0;
}


void Assembler::CheckBuffer() {
  if (buffer_space() <= kGap) {
    GrowBuffer();
  }
  if (pc_offset() >= next_buffer_check_) {
    CheckConstPool(false, true);
  }
}


void Assembler::emit(Instr x) {
  CheckBuffer();
  *reinterpret_cast<Instr*>(pc_) = x;
  pc_ += kInstrSize;
}


Address Assembler::target_pointer_address_at(Address pc) {
  Instr instr = Memory::int32_at(pc);
  return pc + GetLdrRegisterImmediateOffset(instr) + kPcLoadDelta;
}


Address Assembler::target_constant_pool_address_at(
    Address pc, ConstantPoolArray* constant_pool) {
  ASSERT(constant_pool != NULL);
  ASSERT(IsLdrPpImmediateOffset(Memory::int32_at(pc)));
  Instr instr = Memory::int32_at(pc);
  return reinterpret_cast<Address>(constant_pool) +
      GetLdrRegisterImmediateOffset(instr);
}


Address Assembler::target_address_at(Address pc,
                                     ConstantPoolArray* constant_pool) {
  if (IsMovW(Memory::int32_at(pc))) {
    ASSERT(IsMovT(Memory::int32_at(pc + kInstrSize)));
    Instruction* instr = Instruction::At(pc);
    Instruction* next_instr = Instruction::At(pc + kInstrSize);
    return reinterpret_cast<Address>(
        (next_instr->ImmedMovwMovtValue() << 16) |
        instr->ImmedMovwMovtValue());
  } else if (FLAG_enable_ool_constant_pool) {
    ASSERT(IsLdrPpImmediateOffset(Memory::int32_at(pc)));
    return Memory::Address_at(
        target_constant_pool_address_at(pc, constant_pool));
  } else {
    ASSERT(IsLdrPcImmediateOffset(Memory::int32_at(pc)));
    return Memory::Address_at(target_pointer_address_at(pc));
  }
}


Address Assembler::target_address_from_return_address(Address pc) {
  // Returns the address of the call target from the return address that will
  // be returned to after a call.
  // Call sequence on V7 or later is :
  //  movw  ip, #... @ call address low 16
  //  movt  ip, #... @ call address high 16
  //  blx   ip
  //                      @ return address
  // Or pre-V7 or cases that need frequent patching:
  //  ldr   ip, [pc, #...] @ call address
  //  blx   ip
  //                      @ return address
  Address candidate = pc - 2 * Assembler::kInstrSize;
  Instr candidate_instr(Memory::int32_at(candidate));
  if (IsLdrPcImmediateOffset(candidate_instr) |
      IsLdrPpImmediateOffset(candidate_instr)) {
    return candidate;
  }
  candidate = pc - 3 * Assembler::kInstrSize;
  ASSERT(IsMovW(Memory::int32_at(candidate)) &&
         IsMovT(Memory::int32_at(candidate + kInstrSize)));
  return candidate;
}


Address Assembler::return_address_from_call_start(Address pc) {
  if (IsLdrPcImmediateOffset(Memory::int32_at(pc)) |
      IsLdrPpImmediateOffset(Memory::int32_at(pc))) {
    return pc + kInstrSize * 2;
  } else {
    ASSERT(IsMovW(Memory::int32_at(pc)));
    ASSERT(IsMovT(Memory::int32_at(pc + kInstrSize)));
    return pc + kInstrSize * 3;
  }
}


void Assembler::deserialization_set_special_target_at(
    Address constant_pool_entry, Code* code, Address target) {
  if (FLAG_enable_ool_constant_pool) {
    set_target_address_at(constant_pool_entry, code, target);
  } else {
    Memory::Address_at(constant_pool_entry) = target;
  }
}


static Instr EncodeMovwImmediate(uint32_t immediate) {
  ASSERT(immediate < 0x10000);
  return ((immediate & 0xf000) << 4) | (immediate & 0xfff);
}


static Instr PatchMovwImmediate(Instr instruction, uint32_t immediate) {
  instruction &= ~EncodeMovwImmediate(0xffff);
  return instruction | EncodeMovwImmediate(immediate);
}


void Assembler::set_target_address_at(Address pc,
                                      ConstantPoolArray* constant_pool,
                                      Address target,
                                      ICacheFlushMode icache_flush_mode) {
  if (IsMovW(Memory::int32_at(pc))) {
    ASSERT(IsMovT(Memory::int32_at(pc + kInstrSize)));
    uint32_t* instr_ptr = reinterpret_cast<uint32_t*>(pc);
    uint32_t immediate = reinterpret_cast<uint32_t>(target);
    instr_ptr[0] = PatchMovwImmediate(instr_ptr[0], immediate & 0xFFFF);
    instr_ptr[1] = PatchMovwImmediate(instr_ptr[1], immediate >> 16);
    ASSERT(IsMovW(Memory::int32_at(pc)));
    ASSERT(IsMovT(Memory::int32_at(pc + kInstrSize)));
    if (icache_flush_mode != SKIP_ICACHE_FLUSH) {
      CPU::FlushICache(pc, 2 * kInstrSize);
    }
  } else if (FLAG_enable_ool_constant_pool) {
    ASSERT(IsLdrPpImmediateOffset(Memory::int32_at(pc)));
    Memory::Address_at(
      target_constant_pool_address_at(pc, constant_pool)) = target;
  } else {
    ASSERT(IsLdrPcImmediateOffset(Memory::int32_at(pc)));
    Memory::Address_at(target_pointer_address_at(pc)) = target;
    // Intuitively, we would think it is necessary to always flush the
    // instruction cache after patching a target address in the code as follows:
    //   CPU::FlushICache(pc, sizeof(target));
    // However, on ARM, no instruction is actually patched in the case
    // of embedded constants of the form:
    // ldr   ip, [pc, #...]
    // since the instruction accessing this address in the constant pool remains
    // unchanged.
  }
}


} }  // namespace v8::internal

#endif  // V8_ARM_ASSEMBLER_ARM_INL_H_
