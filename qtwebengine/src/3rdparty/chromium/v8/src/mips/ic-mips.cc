// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.



#include "src/v8.h"

#if V8_TARGET_ARCH_MIPS

#include "src/codegen.h"
#include "src/code-stubs.h"
#include "src/ic-inl.h"
#include "src/runtime.h"
#include "src/stub-cache.h"

namespace v8 {
namespace internal {


// ----------------------------------------------------------------------------
// Static IC stub generators.
//

#define __ ACCESS_MASM(masm)


static void GenerateGlobalInstanceTypeCheck(MacroAssembler* masm,
                                            Register type,
                                            Label* global_object) {
  // Register usage:
  //   type: holds the receiver instance type on entry.
  __ Branch(global_object, eq, type, Operand(JS_GLOBAL_OBJECT_TYPE));
  __ Branch(global_object, eq, type, Operand(JS_BUILTINS_OBJECT_TYPE));
  __ Branch(global_object, eq, type, Operand(JS_GLOBAL_PROXY_TYPE));
}


// Generated code falls through if the receiver is a regular non-global
// JS object with slow properties and no interceptors.
static void GenerateNameDictionaryReceiverCheck(MacroAssembler* masm,
                                                Register receiver,
                                                Register elements,
                                                Register scratch0,
                                                Register scratch1,
                                                Label* miss) {
  // Register usage:
  //   receiver: holds the receiver on entry and is unchanged.
  //   elements: holds the property dictionary on fall through.
  // Scratch registers:
  //   scratch0: used to holds the receiver map.
  //   scratch1: used to holds the receiver instance type, receiver bit mask
  //     and elements map.

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(receiver, miss);

  // Check that the receiver is a valid JS object.
  __ GetObjectType(receiver, scratch0, scratch1);
  __ Branch(miss, lt, scratch1, Operand(FIRST_SPEC_OBJECT_TYPE));

  // If this assert fails, we have to check upper bound too.
  STATIC_ASSERT(LAST_TYPE == LAST_SPEC_OBJECT_TYPE);

  GenerateGlobalInstanceTypeCheck(masm, scratch1, miss);

  // Check that the global object does not require access checks.
  __ lbu(scratch1, FieldMemOperand(scratch0, Map::kBitFieldOffset));
  __ And(scratch1, scratch1, Operand((1 << Map::kIsAccessCheckNeeded) |
                           (1 << Map::kHasNamedInterceptor)));
  __ Branch(miss, ne, scratch1, Operand(zero_reg));

  __ lw(elements, FieldMemOperand(receiver, JSObject::kPropertiesOffset));
  __ lw(scratch1, FieldMemOperand(elements, HeapObject::kMapOffset));
  __ LoadRoot(scratch0, Heap::kHashTableMapRootIndex);
  __ Branch(miss, ne, scratch1, Operand(scratch0));
}


// Helper function used from LoadIC GenerateNormal.
//
// elements: Property dictionary. It is not clobbered if a jump to the miss
//           label is done.
// name:     Property name. It is not clobbered if a jump to the miss label is
//           done
// result:   Register for the result. It is only updated if a jump to the miss
//           label is not done. Can be the same as elements or name clobbering
//           one of these in the case of not jumping to the miss label.
// The two scratch registers need to be different from elements, name and
// result.
// The generated code assumes that the receiver has slow properties,
// is not a global object and does not have interceptors.
// The address returned from GenerateStringDictionaryProbes() in scratch2
// is used.
static void GenerateDictionaryLoad(MacroAssembler* masm,
                                   Label* miss,
                                   Register elements,
                                   Register name,
                                   Register result,
                                   Register scratch1,
                                   Register scratch2) {
  // Main use of the scratch registers.
  // scratch1: Used as temporary and to hold the capacity of the property
  //           dictionary.
  // scratch2: Used as temporary.
  Label done;

  // Probe the dictionary.
  NameDictionaryLookupStub::GeneratePositiveLookup(masm,
                                                   miss,
                                                   &done,
                                                   elements,
                                                   name,
                                                   scratch1,
                                                   scratch2);

  // If probing finds an entry check that the value is a normal
  // property.
  __ bind(&done);  // scratch2 == elements + 4 * index.
  const int kElementsStartOffset = NameDictionary::kHeaderSize +
      NameDictionary::kElementsStartIndex * kPointerSize;
  const int kDetailsOffset = kElementsStartOffset + 2 * kPointerSize;
  __ lw(scratch1, FieldMemOperand(scratch2, kDetailsOffset));
  __ And(at,
         scratch1,
         Operand(PropertyDetails::TypeField::kMask << kSmiTagSize));
  __ Branch(miss, ne, at, Operand(zero_reg));

  // Get the value at the masked, scaled index and return.
  __ lw(result,
        FieldMemOperand(scratch2, kElementsStartOffset + 1 * kPointerSize));
}


// Helper function used from StoreIC::GenerateNormal.
//
// elements: Property dictionary. It is not clobbered if a jump to the miss
//           label is done.
// name:     Property name. It is not clobbered if a jump to the miss label is
//           done
// value:    The value to store.
// The two scratch registers need to be different from elements, name and
// result.
// The generated code assumes that the receiver has slow properties,
// is not a global object and does not have interceptors.
// The address returned from GenerateStringDictionaryProbes() in scratch2
// is used.
static void GenerateDictionaryStore(MacroAssembler* masm,
                                    Label* miss,
                                    Register elements,
                                    Register name,
                                    Register value,
                                    Register scratch1,
                                    Register scratch2) {
  // Main use of the scratch registers.
  // scratch1: Used as temporary and to hold the capacity of the property
  //           dictionary.
  // scratch2: Used as temporary.
  Label done;

  // Probe the dictionary.
  NameDictionaryLookupStub::GeneratePositiveLookup(masm,
                                                   miss,
                                                   &done,
                                                   elements,
                                                   name,
                                                   scratch1,
                                                   scratch2);

  // If probing finds an entry in the dictionary check that the value
  // is a normal property that is not read only.
  __ bind(&done);  // scratch2 == elements + 4 * index.
  const int kElementsStartOffset = NameDictionary::kHeaderSize +
      NameDictionary::kElementsStartIndex * kPointerSize;
  const int kDetailsOffset = kElementsStartOffset + 2 * kPointerSize;
  const int kTypeAndReadOnlyMask =
      (PropertyDetails::TypeField::kMask |
       PropertyDetails::AttributesField::encode(READ_ONLY)) << kSmiTagSize;
  __ lw(scratch1, FieldMemOperand(scratch2, kDetailsOffset));
  __ And(at, scratch1, Operand(kTypeAndReadOnlyMask));
  __ Branch(miss, ne, at, Operand(zero_reg));

  // Store the value at the masked, scaled index and return.
  const int kValueOffset = kElementsStartOffset + kPointerSize;
  __ Addu(scratch2, scratch2, Operand(kValueOffset - kHeapObjectTag));
  __ sw(value, MemOperand(scratch2));

  // Update the write barrier. Make sure not to clobber the value.
  __ mov(scratch1, value);
  __ RecordWrite(
      elements, scratch2, scratch1, kRAHasNotBeenSaved, kDontSaveFPRegs);
}


// Checks the receiver for special cases (value type, slow case bits).
// Falls through for regular JS object.
static void GenerateKeyedLoadReceiverCheck(MacroAssembler* masm,
                                           Register receiver,
                                           Register map,
                                           Register scratch,
                                           int interceptor_bit,
                                           Label* slow) {
  // Check that the object isn't a smi.
  __ JumpIfSmi(receiver, slow);
  // Get the map of the receiver.
  __ lw(map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  // Check bit field.
  __ lbu(scratch, FieldMemOperand(map, Map::kBitFieldOffset));
  __ And(at, scratch,
         Operand((1 << Map::kIsAccessCheckNeeded) | (1 << interceptor_bit)));
  __ Branch(slow, ne, at, Operand(zero_reg));
  // Check that the object is some kind of JS object EXCEPT JS Value type.
  // In the case that the object is a value-wrapper object,
  // we enter the runtime system to make sure that indexing into string
  // objects work as intended.
  ASSERT(JS_OBJECT_TYPE > JS_VALUE_TYPE);
  __ lbu(scratch, FieldMemOperand(map, Map::kInstanceTypeOffset));
  __ Branch(slow, lt, scratch, Operand(JS_OBJECT_TYPE));
}


// Loads an indexed element from a fast case array.
// If not_fast_array is NULL, doesn't perform the elements map check.
static void GenerateFastArrayLoad(MacroAssembler* masm,
                                  Register receiver,
                                  Register key,
                                  Register elements,
                                  Register scratch1,
                                  Register scratch2,
                                  Register result,
                                  Label* not_fast_array,
                                  Label* out_of_range) {
  // Register use:
  //
  // receiver - holds the receiver on entry.
  //            Unchanged unless 'result' is the same register.
  //
  // key      - holds the smi key on entry.
  //            Unchanged unless 'result' is the same register.
  //
  // elements - holds the elements of the receiver on exit.
  //
  // result   - holds the result on exit if the load succeeded.
  //            Allowed to be the the same as 'receiver' or 'key'.
  //            Unchanged on bailout so 'receiver' and 'key' can be safely
  //            used by further computation.
  //
  // Scratch registers:
  //
  // scratch1 - used to hold elements map and elements length.
  //            Holds the elements map if not_fast_array branch is taken.
  //
  // scratch2 - used to hold the loaded value.

  __ lw(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  if (not_fast_array != NULL) {
    // Check that the object is in fast mode (not dictionary).
    __ lw(scratch1, FieldMemOperand(elements, HeapObject::kMapOffset));
    __ LoadRoot(at, Heap::kFixedArrayMapRootIndex);
    __ Branch(not_fast_array, ne, scratch1, Operand(at));
  } else {
    __ AssertFastElements(elements);
  }

  // Check that the key (index) is within bounds.
  __ lw(scratch1, FieldMemOperand(elements, FixedArray::kLengthOffset));
  __ Branch(out_of_range, hs, key, Operand(scratch1));

  // Fast case: Do the load.
  __ Addu(scratch1, elements,
          Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  // The key is a smi.
  STATIC_ASSERT(kSmiTag == 0 && kSmiTagSize < kPointerSizeLog2);
  __ sll(at, key, kPointerSizeLog2 - kSmiTagSize);
  __ addu(at, at, scratch1);
  __ lw(scratch2, MemOperand(at));

  __ LoadRoot(at, Heap::kTheHoleValueRootIndex);
  // In case the loaded value is the_hole we have to consult GetProperty
  // to ensure the prototype chain is searched.
  __ Branch(out_of_range, eq, scratch2, Operand(at));
  __ mov(result, scratch2);
}


// Checks whether a key is an array index string or a unique name.
// Falls through if a key is a unique name.
static void GenerateKeyNameCheck(MacroAssembler* masm,
                                 Register key,
                                 Register map,
                                 Register hash,
                                 Label* index_string,
                                 Label* not_unique) {
  // The key is not a smi.
  Label unique;
  // Is it a name?
  __ GetObjectType(key, map, hash);
  __ Branch(not_unique, hi, hash, Operand(LAST_UNIQUE_NAME_TYPE));
  STATIC_ASSERT(LAST_UNIQUE_NAME_TYPE == FIRST_NONSTRING_TYPE);
  __ Branch(&unique, eq, hash, Operand(LAST_UNIQUE_NAME_TYPE));

  // Is the string an array index, with cached numeric value?
  __ lw(hash, FieldMemOperand(key, Name::kHashFieldOffset));
  __ And(at, hash, Operand(Name::kContainsCachedArrayIndexMask));
  __ Branch(index_string, eq, at, Operand(zero_reg));

  // Is the string internalized? We know it's a string, so a single
  // bit test is enough.
  // map: key map
  __ lbu(hash, FieldMemOperand(map, Map::kInstanceTypeOffset));
  STATIC_ASSERT(kInternalizedTag == 0);
  __ And(at, hash, Operand(kIsNotInternalizedMask));
  __ Branch(not_unique, ne, at, Operand(zero_reg));

  __ bind(&unique);
}


void LoadIC::GenerateMegamorphic(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a2    : name
  //  -- ra    : return address
  //  -- a0    : receiver
  // -----------------------------------

  // Probe the stub cache.
  Code::Flags flags = Code::ComputeHandlerFlags(Code::LOAD_IC);
  masm->isolate()->stub_cache()->GenerateProbe(
      masm, flags, a0, a2, a3, t0, t1, t2);

  // Cache miss: Jump to runtime.
  GenerateMiss(masm);
}


void LoadIC::GenerateNormal(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a2    : name
  //  -- lr    : return address
  //  -- a0    : receiver
  // -----------------------------------
  Label miss, slow;

  GenerateNameDictionaryReceiverCheck(masm, a0, a1, a3, t0, &miss);

  // a1: elements
  GenerateDictionaryLoad(masm, &slow, a1, a2, v0, a3, t0);
  __ Ret();

  // Dictionary load failed, go slow (but don't miss).
  __ bind(&slow);
  GenerateRuntimeGetProperty(masm);

  // Cache miss: Jump to runtime.
  __ bind(&miss);
  GenerateMiss(masm);
}


void LoadIC::GenerateMiss(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a2    : name
  //  -- ra    : return address
  //  -- a0    : receiver
  // -----------------------------------
  Isolate* isolate = masm->isolate();

  __ IncrementCounter(isolate->counters()->keyed_load_miss(), 1, a3, t0);

  __ mov(a3, a0);
  __ Push(a3, a2);

  // Perform tail call to the entry.
  ExternalReference ref = ExternalReference(IC_Utility(kLoadIC_Miss), isolate);
  __ TailCallExternalReference(ref, 2, 1);
}


void LoadIC::GenerateRuntimeGetProperty(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- a2    : name
  //  -- ra    : return address
  //  -- a0    : receiver
  // -----------------------------------

  __ mov(a3, a0);
  __ Push(a3, a2);

  __ TailCallRuntime(Runtime::kGetProperty, 2, 1);
}


static MemOperand GenerateMappedArgumentsLookup(MacroAssembler* masm,
                                                Register object,
                                                Register key,
                                                Register scratch1,
                                                Register scratch2,
                                                Register scratch3,
                                                Label* unmapped_case,
                                                Label* slow_case) {
  Heap* heap = masm->isolate()->heap();

  // Check that the receiver is a JSObject. Because of the map check
  // later, we do not need to check for interceptors or whether it
  // requires access checks.
  __ JumpIfSmi(object, slow_case);
  // Check that the object is some kind of JSObject.
  __ GetObjectType(object, scratch1, scratch2);
  __ Branch(slow_case, lt, scratch2, Operand(FIRST_JS_RECEIVER_TYPE));

  // Check that the key is a positive smi.
  __ And(scratch1, key, Operand(0x80000001));
  __ Branch(slow_case, ne, scratch1, Operand(zero_reg));

  // Load the elements into scratch1 and check its map.
  Handle<Map> arguments_map(heap->sloppy_arguments_elements_map());
  __ lw(scratch1, FieldMemOperand(object, JSObject::kElementsOffset));
  __ CheckMap(scratch1,
              scratch2,
              arguments_map,
              slow_case,
              DONT_DO_SMI_CHECK);
  // Check if element is in the range of mapped arguments. If not, jump
  // to the unmapped lookup with the parameter map in scratch1.
  __ lw(scratch2, FieldMemOperand(scratch1, FixedArray::kLengthOffset));
  __ Subu(scratch2, scratch2, Operand(Smi::FromInt(2)));
  __ Branch(unmapped_case, Ugreater_equal, key, Operand(scratch2));

  // Load element index and check whether it is the hole.
  const int kOffset =
      FixedArray::kHeaderSize + 2 * kPointerSize - kHeapObjectTag;

  __ li(scratch3, Operand(kPointerSize >> 1));
  __ Mul(scratch3, key, scratch3);
  __ Addu(scratch3, scratch3, Operand(kOffset));

  __ Addu(scratch2, scratch1, scratch3);
  __ lw(scratch2, MemOperand(scratch2));
  __ LoadRoot(scratch3, Heap::kTheHoleValueRootIndex);
  __ Branch(unmapped_case, eq, scratch2, Operand(scratch3));

  // Load value from context and return it. We can reuse scratch1 because
  // we do not jump to the unmapped lookup (which requires the parameter
  // map in scratch1).
  __ lw(scratch1, FieldMemOperand(scratch1, FixedArray::kHeaderSize));
  __ li(scratch3, Operand(kPointerSize >> 1));
  __ Mul(scratch3, scratch2, scratch3);
  __ Addu(scratch3, scratch3, Operand(Context::kHeaderSize - kHeapObjectTag));
  __ Addu(scratch2, scratch1, scratch3);
  return MemOperand(scratch2);
}


static MemOperand GenerateUnmappedArgumentsLookup(MacroAssembler* masm,
                                                  Register key,
                                                  Register parameter_map,
                                                  Register scratch,
                                                  Label* slow_case) {
  // Element is in arguments backing store, which is referenced by the
  // second element of the parameter_map. The parameter_map register
  // must be loaded with the parameter map of the arguments object and is
  // overwritten.
  const int kBackingStoreOffset = FixedArray::kHeaderSize + kPointerSize;
  Register backing_store = parameter_map;
  __ lw(backing_store, FieldMemOperand(parameter_map, kBackingStoreOffset));
  __ CheckMap(backing_store,
              scratch,
              Heap::kFixedArrayMapRootIndex,
              slow_case,
              DONT_DO_SMI_CHECK);
  __ lw(scratch, FieldMemOperand(backing_store, FixedArray::kLengthOffset));
  __ Branch(slow_case, Ugreater_equal, key, Operand(scratch));
  __ li(scratch, Operand(kPointerSize >> 1));
  __ Mul(scratch, key, scratch);
  __ Addu(scratch,
          scratch,
          Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ Addu(scratch, backing_store, scratch);
  return MemOperand(scratch);
}


void KeyedLoadIC::GenerateSloppyArguments(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- lr     : return address
  //  -- a0     : key
  //  -- a1     : receiver
  // -----------------------------------
  Label slow, notin;
  MemOperand mapped_location =
      GenerateMappedArgumentsLookup(masm, a1, a0, a2, a3, t0, &notin, &slow);
  __ Ret(USE_DELAY_SLOT);
  __ lw(v0, mapped_location);
  __ bind(&notin);
  // The unmapped lookup expects that the parameter map is in a2.
  MemOperand unmapped_location =
      GenerateUnmappedArgumentsLookup(masm, a0, a2, a3, &slow);
  __ lw(a2, unmapped_location);
  __ LoadRoot(a3, Heap::kTheHoleValueRootIndex);
  __ Branch(&slow, eq, a2, Operand(a3));
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, a2);
  __ bind(&slow);
  GenerateMiss(masm);
}


void KeyedStoreIC::GenerateSloppyArguments(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- a0     : value
  //  -- a1     : key
  //  -- a2     : receiver
  //  -- lr     : return address
  // -----------------------------------
  Label slow, notin;
  // Store address is returned in register (of MemOperand) mapped_location.
  MemOperand mapped_location =
      GenerateMappedArgumentsLookup(masm, a2, a1, a3, t0, t1, &notin, &slow);
  __ sw(a0, mapped_location);
  __ mov(t5, a0);
  ASSERT_EQ(mapped_location.offset(), 0);
  __ RecordWrite(a3, mapped_location.rm(), t5,
                 kRAHasNotBeenSaved, kDontSaveFPRegs);
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, a0);  // (In delay slot) return the value stored in v0.
  __ bind(&notin);
  // The unmapped lookup expects that the parameter map is in a3.
  // Store address is returned in register (of MemOperand) unmapped_location.
  MemOperand unmapped_location =
      GenerateUnmappedArgumentsLookup(masm, a1, a3, t0, &slow);
  __ sw(a0, unmapped_location);
  __ mov(t5, a0);
  ASSERT_EQ(unmapped_location.offset(), 0);
  __ RecordWrite(a3, unmapped_location.rm(), t5,
                 kRAHasNotBeenSaved, kDontSaveFPRegs);
  __ Ret(USE_DELAY_SLOT);
  __ mov(v0, a0);  // (In delay slot) return the value stored in v0.
  __ bind(&slow);
  GenerateMiss(masm);
}


void KeyedLoadIC::GenerateMiss(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- ra     : return address
  //  -- a0     : key
  //  -- a1     : receiver
  // -----------------------------------
  Isolate* isolate = masm->isolate();

  __ IncrementCounter(isolate->counters()->keyed_load_miss(), 1, a3, t0);

  __ Push(a1, a0);

  // Perform tail call to the entry.
  ExternalReference ref =
      ExternalReference(IC_Utility(kKeyedLoadIC_Miss), isolate);

  __ TailCallExternalReference(ref, 2, 1);
}


void KeyedLoadIC::GenerateRuntimeGetProperty(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- ra     : return address
  //  -- a0     : key
  //  -- a1     : receiver
  // -----------------------------------

  __ Push(a1, a0);

  __ TailCallRuntime(Runtime::kKeyedGetProperty, 2, 1);
}


void KeyedLoadIC::GenerateGeneric(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- ra     : return address
  //  -- a0     : key
  //  -- a1     : receiver
  // -----------------------------------
  Label slow, check_name, index_smi, index_name, property_array_property;
  Label probe_dictionary, check_number_dictionary;

  Register key = a0;
  Register receiver = a1;

  Isolate* isolate = masm->isolate();

  // Check that the key is a smi.
  __ JumpIfNotSmi(key, &check_name);
  __ bind(&index_smi);
  // Now the key is known to be a smi. This place is also jumped to from below
  // where a numeric string is converted to a smi.

  GenerateKeyedLoadReceiverCheck(
      masm, receiver, a2, a3, Map::kHasIndexedInterceptor, &slow);

  // Check the receiver's map to see if it has fast elements.
  __ CheckFastElements(a2, a3, &check_number_dictionary);

  GenerateFastArrayLoad(
      masm, receiver, key, t0, a3, a2, v0, NULL, &slow);

  __ IncrementCounter(isolate->counters()->keyed_load_generic_smi(), 1, a2, a3);
  __ Ret();

  __ bind(&check_number_dictionary);
  __ lw(t0, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ lw(a3, FieldMemOperand(t0, JSObject::kMapOffset));

  // Check whether the elements is a number dictionary.
  // a0: key
  // a3: elements map
  // t0: elements
  __ LoadRoot(at, Heap::kHashTableMapRootIndex);
  __ Branch(&slow, ne, a3, Operand(at));
  __ sra(a2, a0, kSmiTagSize);
  __ LoadFromNumberDictionary(&slow, t0, a0, v0, a2, a3, t1);
  __ Ret();

  // Slow case, key and receiver still in a0 and a1.
  __ bind(&slow);
  __ IncrementCounter(isolate->counters()->keyed_load_generic_slow(),
                      1,
                      a2,
                      a3);
  GenerateRuntimeGetProperty(masm);

  __ bind(&check_name);
  GenerateKeyNameCheck(masm, key, a2, a3, &index_name, &slow);

  GenerateKeyedLoadReceiverCheck(
       masm, receiver, a2, a3, Map::kHasNamedInterceptor, &slow);


  // If the receiver is a fast-case object, check the keyed lookup
  // cache. Otherwise probe the dictionary.
  __ lw(a3, FieldMemOperand(a1, JSObject::kPropertiesOffset));
  __ lw(t0, FieldMemOperand(a3, HeapObject::kMapOffset));
  __ LoadRoot(at, Heap::kHashTableMapRootIndex);
  __ Branch(&probe_dictionary, eq, t0, Operand(at));

  // Load the map of the receiver, compute the keyed lookup cache hash
  // based on 32 bits of the map pointer and the name hash.
  __ lw(a2, FieldMemOperand(a1, HeapObject::kMapOffset));
  __ sra(a3, a2, KeyedLookupCache::kMapHashShift);
  __ lw(t0, FieldMemOperand(a0, Name::kHashFieldOffset));
  __ sra(at, t0, Name::kHashShift);
  __ xor_(a3, a3, at);
  int mask = KeyedLookupCache::kCapacityMask & KeyedLookupCache::kHashMask;
  __ And(a3, a3, Operand(mask));

  // Load the key (consisting of map and unique name) from the cache and
  // check for match.
  Label load_in_object_property;
  static const int kEntriesPerBucket = KeyedLookupCache::kEntriesPerBucket;
  Label hit_on_nth_entry[kEntriesPerBucket];
  ExternalReference cache_keys =
      ExternalReference::keyed_lookup_cache_keys(isolate);
  __ li(t0, Operand(cache_keys));
  __ sll(at, a3, kPointerSizeLog2 + 1);
  __ addu(t0, t0, at);

  for (int i = 0; i < kEntriesPerBucket - 1; i++) {
    Label try_next_entry;
    __ lw(t1, MemOperand(t0, kPointerSize * i * 2));
    __ Branch(&try_next_entry, ne, a2, Operand(t1));
    __ lw(t1, MemOperand(t0, kPointerSize * (i * 2 + 1)));
    __ Branch(&hit_on_nth_entry[i], eq, a0, Operand(t1));
    __ bind(&try_next_entry);
  }

  __ lw(t1, MemOperand(t0, kPointerSize * (kEntriesPerBucket - 1) * 2));
  __ Branch(&slow, ne, a2, Operand(t1));
  __ lw(t1, MemOperand(t0, kPointerSize * ((kEntriesPerBucket - 1) * 2 + 1)));
  __ Branch(&slow, ne, a0, Operand(t1));

  // Get field offset.
  // a0     : key
  // a1     : receiver
  // a2     : receiver's map
  // a3     : lookup cache index
  ExternalReference cache_field_offsets =
      ExternalReference::keyed_lookup_cache_field_offsets(isolate);

  // Hit on nth entry.
  for (int i = kEntriesPerBucket - 1; i >= 0; i--) {
    __ bind(&hit_on_nth_entry[i]);
    __ li(t0, Operand(cache_field_offsets));
    __ sll(at, a3, kPointerSizeLog2);
    __ addu(at, t0, at);
    __ lw(t1, MemOperand(at, kPointerSize * i));
    __ lbu(t2, FieldMemOperand(a2, Map::kInObjectPropertiesOffset));
    __ Subu(t1, t1, t2);
    __ Branch(&property_array_property, ge, t1, Operand(zero_reg));
    if (i != 0) {
      __ Branch(&load_in_object_property);
    }
  }

  // Load in-object property.
  __ bind(&load_in_object_property);
  __ lbu(t2, FieldMemOperand(a2, Map::kInstanceSizeOffset));
  __ addu(t2, t2, t1);  // Index from start of object.
  __ Subu(a1, a1, Operand(kHeapObjectTag));  // Remove the heap tag.
  __ sll(at, t2, kPointerSizeLog2);
  __ addu(at, a1, at);
  __ lw(v0, MemOperand(at));
  __ IncrementCounter(isolate->counters()->keyed_load_generic_lookup_cache(),
                      1,
                      a2,
                      a3);
  __ Ret();

  // Load property array property.
  __ bind(&property_array_property);
  __ lw(a1, FieldMemOperand(a1, JSObject::kPropertiesOffset));
  __ Addu(a1, a1, FixedArray::kHeaderSize - kHeapObjectTag);
  __ sll(t0, t1, kPointerSizeLog2);
  __ Addu(t0, t0, a1);
  __ lw(v0, MemOperand(t0));
  __ IncrementCounter(isolate->counters()->keyed_load_generic_lookup_cache(),
                      1,
                      a2,
                      a3);
  __ Ret();


  // Do a quick inline probe of the receiver's dictionary, if it
  // exists.
  __ bind(&probe_dictionary);
  // a1: receiver
  // a0: key
  // a3: elements
  __ lw(a2, FieldMemOperand(a1, HeapObject::kMapOffset));
  __ lbu(a2, FieldMemOperand(a2, Map::kInstanceTypeOffset));
  GenerateGlobalInstanceTypeCheck(masm, a2, &slow);
  // Load the property to v0.
  GenerateDictionaryLoad(masm, &slow, a3, a0, v0, a2, t0);
  __ IncrementCounter(isolate->counters()->keyed_load_generic_symbol(),
                      1,
                      a2,
                      a3);
  __ Ret();

  __ bind(&index_name);
  __ IndexFromHash(a3, key);
  // Now jump to the place where smi keys are handled.
  __ Branch(&index_smi);
}


void KeyedLoadIC::GenerateString(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- ra     : return address
  //  -- a0     : key (index)
  //  -- a1     : receiver
  // -----------------------------------
  Label miss;

  Register receiver = a1;
  Register index = a0;
  Register scratch = a3;
  Register result = v0;

  StringCharAtGenerator char_at_generator(receiver,
                                          index,
                                          scratch,
                                          result,
                                          &miss,  // When not a string.
                                          &miss,  // When not a number.
                                          &miss,  // When index out of range.
                                          STRING_INDEX_IS_ARRAY_INDEX);
  char_at_generator.GenerateFast(masm);
  __ Ret();

  StubRuntimeCallHelper call_helper;
  char_at_generator.GenerateSlow(masm, call_helper);

  __ bind(&miss);
  GenerateMiss(masm);
}


void KeyedStoreIC::GenerateRuntimeSetProperty(MacroAssembler* masm,
                                              StrictMode strict_mode) {
  // ---------- S t a t e --------------
  //  -- a0     : value
  //  -- a1     : key
  //  -- a2     : receiver
  //  -- ra     : return address
  // -----------------------------------

  // Push receiver, key and value for runtime call.
  __ Push(a2, a1, a0);
  __ li(a1, Operand(Smi::FromInt(NONE)));          // PropertyAttributes.
  __ li(a0, Operand(Smi::FromInt(strict_mode)));   // Strict mode.
  __ Push(a1, a0);

  __ TailCallRuntime(Runtime::kSetProperty, 5, 1);
}


static void KeyedStoreGenerateGenericHelper(
    MacroAssembler* masm,
    Label* fast_object,
    Label* fast_double,
    Label* slow,
    KeyedStoreCheckMap check_map,
    KeyedStoreIncrementLength increment_length,
    Register value,
    Register key,
    Register receiver,
    Register receiver_map,
    Register elements_map,
    Register elements) {
  Label transition_smi_elements;
  Label finish_object_store, non_double_value, transition_double_elements;
  Label fast_double_without_map_check;

  // Fast case: Do the store, could be either Object or double.
  __ bind(fast_object);
  Register scratch_value = t0;
  Register address = t1;
  if (check_map == kCheckMap) {
    __ lw(elements_map, FieldMemOperand(elements, HeapObject::kMapOffset));
    __ Branch(fast_double, ne, elements_map,
              Operand(masm->isolate()->factory()->fixed_array_map()));
  }

  // HOLECHECK: guards "A[i] = V"
  // We have to go to the runtime if the current value is the hole because
  // there may be a callback on the element.
  Label holecheck_passed1;
  __ Addu(address, elements, FixedArray::kHeaderSize - kHeapObjectTag);
  __ sll(at, key, kPointerSizeLog2 - kSmiTagSize);
  __ addu(address, address, at);
  __ lw(scratch_value, MemOperand(address));
  __ Branch(&holecheck_passed1, ne, scratch_value,
            Operand(masm->isolate()->factory()->the_hole_value()));
  __ JumpIfDictionaryInPrototypeChain(receiver, elements_map, scratch_value,
                                      slow);

  __ bind(&holecheck_passed1);

  // Smi stores don't require further checks.
  Label non_smi_value;
  __ JumpIfNotSmi(value, &non_smi_value);

  if (increment_length == kIncrementLength) {
    // Add 1 to receiver->length.
    __ Addu(scratch_value, key, Operand(Smi::FromInt(1)));
    __ sw(scratch_value, FieldMemOperand(receiver, JSArray::kLengthOffset));
  }
  // It's irrelevant whether array is smi-only or not when writing a smi.
  __ Addu(address, elements, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ sll(scratch_value, key, kPointerSizeLog2 - kSmiTagSize);
  __ Addu(address, address, scratch_value);
  __ sw(value, MemOperand(address));
  __ Ret();

  __ bind(&non_smi_value);
  // Escape to elements kind transition case.
  __ CheckFastObjectElements(receiver_map, scratch_value,
                             &transition_smi_elements);

  // Fast elements array, store the value to the elements backing store.
  __ bind(&finish_object_store);
  if (increment_length == kIncrementLength) {
    // Add 1 to receiver->length.
    __ Addu(scratch_value, key, Operand(Smi::FromInt(1)));
    __ sw(scratch_value, FieldMemOperand(receiver, JSArray::kLengthOffset));
  }
  __ Addu(address, elements, Operand(FixedArray::kHeaderSize - kHeapObjectTag));
  __ sll(scratch_value, key, kPointerSizeLog2 - kSmiTagSize);
  __ Addu(address, address, scratch_value);
  __ sw(value, MemOperand(address));
  // Update write barrier for the elements array address.
  __ mov(scratch_value, value);  // Preserve the value which is returned.
  __ RecordWrite(elements,
                 address,
                 scratch_value,
                 kRAHasNotBeenSaved,
                 kDontSaveFPRegs,
                 EMIT_REMEMBERED_SET,
                 OMIT_SMI_CHECK);
  __ Ret();

  __ bind(fast_double);
  if (check_map == kCheckMap) {
    // Check for fast double array case. If this fails, call through to the
    // runtime.
    __ LoadRoot(at, Heap::kFixedDoubleArrayMapRootIndex);
    __ Branch(slow, ne, elements_map, Operand(at));
  }

  // HOLECHECK: guards "A[i] double hole?"
  // We have to see if the double version of the hole is present. If so
  // go to the runtime.
  __ Addu(address, elements,
          Operand(FixedDoubleArray::kHeaderSize + kHoleNanUpper32Offset
                  - kHeapObjectTag));
  __ sll(at, key, kPointerSizeLog2);
  __ addu(address, address, at);
  __ lw(scratch_value, MemOperand(address));
  __ Branch(&fast_double_without_map_check, ne, scratch_value,
            Operand(kHoleNanUpper32));
  __ JumpIfDictionaryInPrototypeChain(receiver, elements_map, scratch_value,
                                      slow);

  __ bind(&fast_double_without_map_check);
  __ StoreNumberToDoubleElements(value,
                                 key,
                                 elements,  // Overwritten.
                                 a3,        // Scratch regs...
                                 t0,
                                 t1,
                                 &transition_double_elements);
  if (increment_length == kIncrementLength) {
    // Add 1 to receiver->length.
    __ Addu(scratch_value, key, Operand(Smi::FromInt(1)));
    __ sw(scratch_value, FieldMemOperand(receiver, JSArray::kLengthOffset));
  }
  __ Ret();

  __ bind(&transition_smi_elements);
  // Transition the array appropriately depending on the value type.
  __ lw(t0, FieldMemOperand(value, HeapObject::kMapOffset));
  __ LoadRoot(at, Heap::kHeapNumberMapRootIndex);
  __ Branch(&non_double_value, ne, t0, Operand(at));

  // Value is a double. Transition FAST_SMI_ELEMENTS ->
  // FAST_DOUBLE_ELEMENTS and complete the store.
  __ LoadTransitionedArrayMapConditional(FAST_SMI_ELEMENTS,
                                         FAST_DOUBLE_ELEMENTS,
                                         receiver_map,
                                         t0,
                                         slow);
  ASSERT(receiver_map.is(a3));  // Transition code expects map in a3
  AllocationSiteMode mode = AllocationSite::GetMode(FAST_SMI_ELEMENTS,
                                                    FAST_DOUBLE_ELEMENTS);
  ElementsTransitionGenerator::GenerateSmiToDouble(masm, mode, slow);
  __ lw(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ jmp(&fast_double_without_map_check);

  __ bind(&non_double_value);
  // Value is not a double, FAST_SMI_ELEMENTS -> FAST_ELEMENTS
  __ LoadTransitionedArrayMapConditional(FAST_SMI_ELEMENTS,
                                         FAST_ELEMENTS,
                                         receiver_map,
                                         t0,
                                         slow);
  ASSERT(receiver_map.is(a3));  // Transition code expects map in a3
  mode = AllocationSite::GetMode(FAST_SMI_ELEMENTS, FAST_ELEMENTS);
  ElementsTransitionGenerator::GenerateMapChangeElementsTransition(masm, mode,
                                                                   slow);
  __ lw(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ jmp(&finish_object_store);

  __ bind(&transition_double_elements);
  // Elements are FAST_DOUBLE_ELEMENTS, but value is an Object that's not a
  // HeapNumber. Make sure that the receiver is a Array with FAST_ELEMENTS and
  // transition array from FAST_DOUBLE_ELEMENTS to FAST_ELEMENTS
  __ LoadTransitionedArrayMapConditional(FAST_DOUBLE_ELEMENTS,
                                         FAST_ELEMENTS,
                                         receiver_map,
                                         t0,
                                         slow);
  ASSERT(receiver_map.is(a3));  // Transition code expects map in a3
  mode = AllocationSite::GetMode(FAST_DOUBLE_ELEMENTS, FAST_ELEMENTS);
  ElementsTransitionGenerator::GenerateDoubleToObject(masm, mode, slow);
  __ lw(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ jmp(&finish_object_store);
}


void KeyedStoreIC::GenerateGeneric(MacroAssembler* masm,
                                   StrictMode strict_mode) {
  // ---------- S t a t e --------------
  //  -- a0     : value
  //  -- a1     : key
  //  -- a2     : receiver
  //  -- ra     : return address
  // -----------------------------------
  Label slow, fast_object, fast_object_grow;
  Label fast_double, fast_double_grow;
  Label array, extra, check_if_double_array;

  // Register usage.
  Register value = a0;
  Register key = a1;
  Register receiver = a2;
  Register receiver_map = a3;
  Register elements_map = t2;
  Register elements = t3;  // Elements array of the receiver.
  // t0 and t1 are used as general scratch registers.

  // Check that the key is a smi.
  __ JumpIfNotSmi(key, &slow);
  // Check that the object isn't a smi.
  __ JumpIfSmi(receiver, &slow);
  // Get the map of the object.
  __ lw(receiver_map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  // Check that the receiver does not require access checks and is not observed.
  // The generic stub does not perform map checks or handle observed objects.
  __ lbu(t0, FieldMemOperand(receiver_map, Map::kBitFieldOffset));
  __ And(t0, t0, Operand(1 << Map::kIsAccessCheckNeeded |
                         1 << Map::kIsObserved));
  __ Branch(&slow, ne, t0, Operand(zero_reg));
  // Check if the object is a JS array or not.
  __ lbu(t0, FieldMemOperand(receiver_map, Map::kInstanceTypeOffset));
  __ Branch(&array, eq, t0, Operand(JS_ARRAY_TYPE));
  // Check that the object is some kind of JSObject.
  __ Branch(&slow, lt, t0, Operand(FIRST_JS_OBJECT_TYPE));

  // Object case: Check key against length in the elements array.
  __ lw(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  // Check array bounds. Both the key and the length of FixedArray are smis.
  __ lw(t0, FieldMemOperand(elements, FixedArray::kLengthOffset));
  __ Branch(&fast_object, lo, key, Operand(t0));

  // Slow case, handle jump to runtime.
  __ bind(&slow);
  // Entry registers are intact.
  // a0: value.
  // a1: key.
  // a2: receiver.
  GenerateRuntimeSetProperty(masm, strict_mode);

  // Extra capacity case: Check if there is extra capacity to
  // perform the store and update the length. Used for adding one
  // element to the array by writing to array[array.length].
  __ bind(&extra);
  // Condition code from comparing key and array length is still available.
  // Only support writing to array[array.length].
  __ Branch(&slow, ne, key, Operand(t0));
  // Check for room in the elements backing store.
  // Both the key and the length of FixedArray are smis.
  __ lw(t0, FieldMemOperand(elements, FixedArray::kLengthOffset));
  __ Branch(&slow, hs, key, Operand(t0));
  __ lw(elements_map, FieldMemOperand(elements, HeapObject::kMapOffset));
  __ Branch(
      &check_if_double_array, ne, elements_map, Heap::kFixedArrayMapRootIndex);

  __ jmp(&fast_object_grow);

  __ bind(&check_if_double_array);
  __ Branch(&slow, ne, elements_map, Heap::kFixedDoubleArrayMapRootIndex);
  __ jmp(&fast_double_grow);

  // Array case: Get the length and the elements array from the JS
  // array. Check that the array is in fast mode (and writable); if it
  // is the length is always a smi.
  __ bind(&array);
  __ lw(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));

  // Check the key against the length in the array.
  __ lw(t0, FieldMemOperand(receiver, JSArray::kLengthOffset));
  __ Branch(&extra, hs, key, Operand(t0));

  KeyedStoreGenerateGenericHelper(masm, &fast_object, &fast_double,
                                  &slow, kCheckMap, kDontIncrementLength,
                                  value, key, receiver, receiver_map,
                                  elements_map, elements);
  KeyedStoreGenerateGenericHelper(masm, &fast_object_grow, &fast_double_grow,
                                  &slow, kDontCheckMap, kIncrementLength,
                                  value, key, receiver, receiver_map,
                                  elements_map, elements);
}


void KeyedLoadIC::GenerateIndexedInterceptor(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- ra     : return address
  //  -- a0     : key
  //  -- a1     : receiver
  // -----------------------------------
  Label slow;

  // Check that the receiver isn't a smi.
  __ JumpIfSmi(a1, &slow);

  // Check that the key is an array index, that is Uint32.
  __ And(t0, a0, Operand(kSmiTagMask | kSmiSignMask));
  __ Branch(&slow, ne, t0, Operand(zero_reg));

  // Get the map of the receiver.
  __ lw(a2, FieldMemOperand(a1, HeapObject::kMapOffset));

  // Check that it has indexed interceptor and access checks
  // are not enabled for this object.
  __ lbu(a3, FieldMemOperand(a2, Map::kBitFieldOffset));
  __ And(a3, a3, Operand(kSlowCaseBitFieldMask));
  __ Branch(&slow, ne, a3, Operand(1 << Map::kHasIndexedInterceptor));
  // Everything is fine, call runtime.
  __ Push(a1, a0);  // Receiver, key.

  // Perform tail call to the entry.
  __ TailCallExternalReference(ExternalReference(
       IC_Utility(kKeyedLoadPropertyWithInterceptor), masm->isolate()), 2, 1);

  __ bind(&slow);
  GenerateMiss(masm);
}


void KeyedStoreIC::GenerateMiss(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- a0     : value
  //  -- a1     : key
  //  -- a2     : receiver
  //  -- ra     : return address
  // -----------------------------------

  // Push receiver, key and value for runtime call.
  __ Push(a2, a1, a0);

  ExternalReference ref =
      ExternalReference(IC_Utility(kKeyedStoreIC_Miss), masm->isolate());
  __ TailCallExternalReference(ref, 3, 1);
}


void StoreIC::GenerateSlow(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- a0     : value
  //  -- a2     : key
  //  -- a1     : receiver
  //  -- ra     : return address
  // -----------------------------------

  // Push receiver, key and value for runtime call.
  __ Push(a1, a2, a0);

  // The slow case calls into the runtime to complete the store without causing
  // an IC miss that would otherwise cause a transition to the generic stub.
  ExternalReference ref =
      ExternalReference(IC_Utility(kStoreIC_Slow), masm->isolate());
  __ TailCallExternalReference(ref, 3, 1);
}


void KeyedStoreIC::GenerateSlow(MacroAssembler* masm) {
  // ---------- S t a t e --------------
  //  -- a0     : value
  //  -- a1     : key
  //  -- a2     : receiver
  //  -- ra     : return address
  // -----------------------------------

  // Push receiver, key and value for runtime call.
  // We can't use MultiPush as the order of the registers is important.
  __ Push(a2, a1, a0);

  // The slow case calls into the runtime to complete the store without causing
  // an IC miss that would otherwise cause a transition to the generic stub.
  ExternalReference ref =
      ExternalReference(IC_Utility(kKeyedStoreIC_Slow), masm->isolate());

  __ TailCallExternalReference(ref, 3, 1);
}


void StoreIC::GenerateMegamorphic(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a0    : value
  //  -- a1    : receiver
  //  -- a2    : name
  //  -- ra    : return address
  // -----------------------------------

  // Get the receiver from the stack and probe the stub cache.
  Code::Flags flags = Code::ComputeHandlerFlags(Code::STORE_IC);
  masm->isolate()->stub_cache()->GenerateProbe(
      masm, flags, a1, a2, a3, t0, t1, t2);

  // Cache miss: Jump to runtime.
  GenerateMiss(masm);
}


void StoreIC::GenerateMiss(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a0    : value
  //  -- a1    : receiver
  //  -- a2    : name
  //  -- ra    : return address
  // -----------------------------------

  __ Push(a1, a2, a0);
  // Perform tail call to the entry.
  ExternalReference ref = ExternalReference(IC_Utility(kStoreIC_Miss),
                                            masm->isolate());
  __ TailCallExternalReference(ref, 3, 1);
}


void StoreIC::GenerateNormal(MacroAssembler* masm) {
  // ----------- S t a t e -------------
  //  -- a0    : value
  //  -- a1    : receiver
  //  -- a2    : name
  //  -- ra    : return address
  // -----------------------------------
  Label miss;

  GenerateNameDictionaryReceiverCheck(masm, a1, a3, t0, t1, &miss);

  GenerateDictionaryStore(masm, &miss, a3, a2, a0, t0, t1);
  Counters* counters = masm->isolate()->counters();
  __ IncrementCounter(counters->store_normal_hit(), 1, t0, t1);
  __ Ret();

  __ bind(&miss);
  __ IncrementCounter(counters->store_normal_miss(), 1, t0, t1);
  GenerateMiss(masm);
}


void StoreIC::GenerateRuntimeSetProperty(MacroAssembler* masm,
                                         StrictMode strict_mode) {
  // ----------- S t a t e -------------
  //  -- a0    : value
  //  -- a1    : receiver
  //  -- a2    : name
  //  -- ra    : return address
  // -----------------------------------

  __ Push(a1, a2, a0);

  __ li(a1, Operand(Smi::FromInt(NONE)));  // PropertyAttributes.
  __ li(a0, Operand(Smi::FromInt(strict_mode)));
  __ Push(a1, a0);

  // Do tail-call to runtime routine.
  __ TailCallRuntime(Runtime::kSetProperty, 5, 1);
}


#undef __


Condition CompareIC::ComputeCondition(Token::Value op) {
  switch (op) {
    case Token::EQ_STRICT:
    case Token::EQ:
      return eq;
    case Token::LT:
      return lt;
    case Token::GT:
      return gt;
    case Token::LTE:
      return le;
    case Token::GTE:
      return ge;
    default:
      UNREACHABLE();
      return kNoCondition;
  }
}


bool CompareIC::HasInlinedSmiCode(Address address) {
  // The address of the instruction following the call.
  Address andi_instruction_address =
      address + Assembler::kCallTargetAddressOffset;

  // If the instruction following the call is not a andi at, rx, #yyy, nothing
  // was inlined.
  Instr instr = Assembler::instr_at(andi_instruction_address);
  return Assembler::IsAndImmediate(instr) &&
      Assembler::GetRt(instr) == static_cast<uint32_t>(zero_reg.code());
}


void PatchInlinedSmiCode(Address address, InlinedSmiCheck check) {
  Address andi_instruction_address =
      address + Assembler::kCallTargetAddressOffset;

  // If the instruction following the call is not a andi at, rx, #yyy, nothing
  // was inlined.
  Instr instr = Assembler::instr_at(andi_instruction_address);
  if (!(Assembler::IsAndImmediate(instr) &&
        Assembler::GetRt(instr) == static_cast<uint32_t>(zero_reg.code()))) {
    return;
  }

  // The delta to the start of the map check instruction and the
  // condition code uses at the patched jump.
  int delta = Assembler::GetImmediate16(instr);
  delta += Assembler::GetRs(instr) * kImm16Mask;
  // If the delta is 0 the instruction is andi at, zero_reg, #0 which also
  // signals that nothing was inlined.
  if (delta == 0) {
    return;
  }

  if (FLAG_trace_ic) {
    PrintF("[  patching ic at %p, andi=%p, delta=%d\n",
           address, andi_instruction_address, delta);
  }

  Address patch_address =
      andi_instruction_address - delta * Instruction::kInstrSize;
  Instr instr_at_patch = Assembler::instr_at(patch_address);
  Instr branch_instr =
      Assembler::instr_at(patch_address + Instruction::kInstrSize);
  // This is patching a conditional "jump if not smi/jump if smi" site.
  // Enabling by changing from
  //   andi at, rx, 0
  //   Branch <target>, eq, at, Operand(zero_reg)
  // to:
  //   andi at, rx, #kSmiTagMask
  //   Branch <target>, ne, at, Operand(zero_reg)
  // and vice-versa to be disabled again.
  CodePatcher patcher(patch_address, 2);
  Register reg = Register::from_code(Assembler::GetRs(instr_at_patch));
  if (check == ENABLE_INLINED_SMI_CHECK) {
    ASSERT(Assembler::IsAndImmediate(instr_at_patch));
    ASSERT_EQ(0, Assembler::GetImmediate16(instr_at_patch));
    patcher.masm()->andi(at, reg, kSmiTagMask);
  } else {
    ASSERT(check == DISABLE_INLINED_SMI_CHECK);
    ASSERT(Assembler::IsAndImmediate(instr_at_patch));
    patcher.masm()->andi(at, reg, 0);
  }
  ASSERT(Assembler::IsBranch(branch_instr));
  if (Assembler::IsBeq(branch_instr)) {
    patcher.ChangeBranchCondition(ne);
  } else {
    ASSERT(Assembler::IsBne(branch_instr));
    patcher.ChangeBranchCondition(eq);
  }
}


} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_MIPS
