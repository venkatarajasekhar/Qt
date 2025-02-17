// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_INCREMENTAL_MARKING_H_
#define V8_INCREMENTAL_MARKING_H_


#include "src/execution.h"
#include "src/mark-compact.h"
#include "src/objects.h"

namespace v8 {
namespace internal {


class IncrementalMarking {
 public:
  enum State {
    STOPPED,
    SWEEPING,
    MARKING,
    COMPLETE
  };

  enum CompletionAction {
    GC_VIA_STACK_GUARD,
    NO_GC_VIA_STACK_GUARD
  };

  explicit IncrementalMarking(Heap* heap);

  static void Initialize();

  void TearDown();

  State state() {
    ASSERT(state_ == STOPPED || FLAG_incremental_marking);
    return state_;
  }

  bool should_hurry() { return should_hurry_; }
  void set_should_hurry(bool val) { should_hurry_ = val; }

  inline bool IsStopped() { return state() == STOPPED; }

  INLINE(bool IsMarking()) { return state() >= MARKING; }

  inline bool IsMarkingIncomplete() { return state() == MARKING; }

  inline bool IsComplete() { return state() == COMPLETE; }

  bool WorthActivating();

  enum CompactionFlag { ALLOW_COMPACTION, PREVENT_COMPACTION };

  void Start(CompactionFlag flag = ALLOW_COMPACTION);

  void Stop();

  void PrepareForScavenge();

  void UpdateMarkingDequeAfterScavenge();

  void Hurry();

  void Finalize();

  void Abort();

  void MarkingComplete(CompletionAction action);

  // It's hard to know how much work the incremental marker should do to make
  // progress in the face of the mutator creating new work for it.  We start
  // of at a moderate rate of work and gradually increase the speed of the
  // incremental marker until it completes.
  // Do some marking every time this much memory has been allocated or that many
  // heavy (color-checking) write barriers have been invoked.
  static const intptr_t kAllocatedThreshold = 65536;
  static const intptr_t kWriteBarriersInvokedThreshold = 32768;
  // Start off by marking this many times more memory than has been allocated.
  static const intptr_t kInitialMarkingSpeed = 1;
  // But if we are promoting a lot of data we need to mark faster to keep up
  // with the data that is entering the old space through promotion.
  static const intptr_t kFastMarking = 3;
  // After this many steps we increase the marking/allocating factor.
  static const intptr_t kMarkingSpeedAccellerationInterval = 1024;
  // This is how much we increase the marking/allocating factor by.
  static const intptr_t kMarkingSpeedAccelleration = 2;
  static const intptr_t kMaxMarkingSpeed = 1000;

  void OldSpaceStep(intptr_t allocated);

  void Step(intptr_t allocated, CompletionAction action);

  inline void RestartIfNotMarking() {
    if (state_ == COMPLETE) {
      state_ = MARKING;
      if (FLAG_trace_incremental_marking) {
        PrintF("[IncrementalMarking] Restarting (new grey objects)\n");
      }
    }
  }

  static void RecordWriteFromCode(HeapObject* obj,
                                  Object** slot,
                                  Isolate* isolate);

  // Record a slot for compaction.  Returns false for objects that are
  // guaranteed to be rescanned or not guaranteed to survive.
  //
  // No slots in white objects should be recorded, as some slots are typed and
  // cannot be interpreted correctly if the underlying object does not survive
  // the incremental cycle (stays white).
  INLINE(bool BaseRecordWrite(HeapObject* obj, Object** slot, Object* value));
  INLINE(void RecordWrite(HeapObject* obj, Object** slot, Object* value));
  INLINE(void RecordWriteIntoCode(HeapObject* obj,
                                  RelocInfo* rinfo,
                                  Object* value));
  INLINE(void RecordWriteOfCodeEntry(JSFunction* host,
                                     Object** slot,
                                     Code* value));


  void RecordWriteSlow(HeapObject* obj, Object** slot, Object* value);
  void RecordWriteIntoCodeSlow(HeapObject* obj,
                               RelocInfo* rinfo,
                               Object* value);
  void RecordWriteOfCodeEntrySlow(JSFunction* host, Object** slot, Code* value);
  void RecordCodeTargetPatch(Code* host, Address pc, HeapObject* value);
  void RecordCodeTargetPatch(Address pc, HeapObject* value);

  inline void RecordWrites(HeapObject* obj);

  inline void BlackToGreyAndUnshift(HeapObject* obj, MarkBit mark_bit);

  inline void WhiteToGreyAndPush(HeapObject* obj, MarkBit mark_bit);

  inline int steps_count() {
    return steps_count_;
  }

  inline double steps_took() {
    return steps_took_;
  }

  inline double longest_step() {
    return longest_step_;
  }

  inline int steps_count_since_last_gc() {
    return steps_count_since_last_gc_;
  }

  inline double steps_took_since_last_gc() {
    return steps_took_since_last_gc_;
  }

  inline void SetOldSpacePageFlags(MemoryChunk* chunk) {
    SetOldSpacePageFlags(chunk, IsMarking(), IsCompacting());
  }

  inline void SetNewSpacePageFlags(NewSpacePage* chunk) {
    SetNewSpacePageFlags(chunk, IsMarking());
  }

  MarkingDeque* marking_deque() { return &marking_deque_; }

  bool IsCompacting() { return IsMarking() && is_compacting_; }

  void ActivateGeneratedStub(Code* stub);

  void NotifyOfHighPromotionRate() {
    if (IsMarking()) {
      if (marking_speed_ < kFastMarking) {
        if (FLAG_trace_gc) {
          PrintPID("Increasing marking speed to %d "
                   "due to high promotion rate\n",
                   static_cast<int>(kFastMarking));
        }
        marking_speed_ = kFastMarking;
      }
    }
  }

  void EnterNoMarkingScope() {
    no_marking_scope_depth_++;
  }

  void LeaveNoMarkingScope() {
    no_marking_scope_depth_--;
  }

  void UncommitMarkingDeque();

  void NotifyIncompleteScanOfObject(int unscanned_bytes) {
    unscanned_bytes_of_large_object_ = unscanned_bytes;
  }

 private:
  int64_t SpaceLeftInOldSpace();

  void ResetStepCounters();

  void StartMarking(CompactionFlag flag);

  void ActivateIncrementalWriteBarrier(PagedSpace* space);
  static void ActivateIncrementalWriteBarrier(NewSpace* space);
  void ActivateIncrementalWriteBarrier();

  static void DeactivateIncrementalWriteBarrierForSpace(PagedSpace* space);
  static void DeactivateIncrementalWriteBarrierForSpace(NewSpace* space);
  void DeactivateIncrementalWriteBarrier();

  static void SetOldSpacePageFlags(MemoryChunk* chunk,
                                   bool is_marking,
                                   bool is_compacting);

  static void SetNewSpacePageFlags(NewSpacePage* chunk, bool is_marking);

  void EnsureMarkingDequeIsCommitted();

  INLINE(void ProcessMarkingDeque());

  INLINE(void ProcessMarkingDeque(intptr_t bytes_to_process));

  INLINE(void VisitObject(Map* map, HeapObject* obj, int size));

  Heap* heap_;

  State state_;
  bool is_compacting_;

  VirtualMemory* marking_deque_memory_;
  bool marking_deque_memory_committed_;
  MarkingDeque marking_deque_;

  int steps_count_;
  double steps_took_;
  double longest_step_;
  int64_t old_generation_space_available_at_start_of_incremental_;
  int64_t old_generation_space_used_at_start_of_incremental_;
  int steps_count_since_last_gc_;
  double steps_took_since_last_gc_;
  int64_t bytes_rescanned_;
  bool should_hurry_;
  int marking_speed_;
  intptr_t bytes_scanned_;
  intptr_t allocated_;
  intptr_t write_barriers_invoked_since_last_step_;

  int no_marking_scope_depth_;

  int unscanned_bytes_of_large_object_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(IncrementalMarking);
};

} }  // namespace v8::internal

#endif  // V8_INCREMENTAL_MARKING_H_
