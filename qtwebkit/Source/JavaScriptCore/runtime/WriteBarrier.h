/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WriteBarrier_h
#define WriteBarrier_h

#include "GCAssertions.h"
#include "HandleTypes.h"
#include "Heap.h"
#include "SamplingCounter.h"
#include <wtf/TypeTraits.h>

namespace JSC {

class JSCell;
class VM;
class JSGlobalObject;

template<class T> class WriteBarrierBase;
template<> class WriteBarrierBase<JSValue>;

JS_EXPORT_PRIVATE void slowValidateCell(JSCell*);
JS_EXPORT_PRIVATE void slowValidateCell(JSGlobalObject*);
    
#if ENABLE(GC_VALIDATION)
template<class T> inline void validateCell(T cell)
{
    ASSERT_GC_OBJECT_INHERITS(cell, &WTF::RemovePointer<T>::Type::s_info);
}

template<> inline void validateCell<JSCell*>(JSCell* cell)
{
    slowValidateCell(cell);
}

template<> inline void validateCell<JSGlobalObject*>(JSGlobalObject* globalObject)
{
    slowValidateCell(globalObject);
}
#else
template<class T> inline void validateCell(T)
{
}
#endif

// We have a separate base class with no constructors for use in Unions.
template <typename T> class WriteBarrierBase {
public:
    void set(VM& vm, const JSCell* owner, T* value)
    {
        ASSERT(value);
        validateCell(value);
        setEarlyValue(vm, owner, value);
    }
    
    // This is meant to be used like operator=, but is called copyFrom instead, in
    // order to kindly inform the C++ compiler that its advice is not appreciated.
    void copyFrom(const WriteBarrierBase<T>& other)
    {
        m_cell = other.m_cell;
    }

    void setMayBeNull(VM& vm, const JSCell* owner, T* value)
    {
        if (value)
            validateCell(value);
        setEarlyValue(vm, owner, value);
    }

    // Should only be used by JSCell during early initialisation
    // when some basic types aren't yet completely instantiated
    void setEarlyValue(VM&, const JSCell* owner, T* value)
    {
        this->m_cell = reinterpret_cast<JSCell*>(value);
        Heap::writeBarrier(owner, this->m_cell);
    }
    
    T* get() const
    {
        // Copy m_cell to a local to avoid multiple-read issues. (See <http://webkit.org/b/110854>)
        JSCell* cell = m_cell;
        if (cell)
            validateCell(cell);
        return reinterpret_cast<T*>(static_cast<void*>(cell));
    }

    T* operator*() const
    {
        ASSERT(m_cell);
        validateCell<T>(static_cast<T*>(m_cell));
        return static_cast<T*>(m_cell);
    }

    T* operator->() const
    {
        ASSERT(m_cell);
        validateCell(static_cast<T*>(m_cell));
        return static_cast<T*>(m_cell);
    }

    void clear() { m_cell = 0; }
    
    JSCell** slot() { return &m_cell; }
    
    typedef T* (WriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return m_cell ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    
    bool operator!() const { return !m_cell; }

    void setWithoutWriteBarrier(T* value)
    {
#if ENABLE(WRITE_BARRIER_PROFILING)
        WriteBarrierCounters::usesWithoutBarrierFromCpp.count();
#endif
        this->m_cell = reinterpret_cast<JSCell*>(value);
    }

#if ENABLE(GC_VALIDATION)
    T* unvalidatedGet() const { return reinterpret_cast<T*>(static_cast<void*>(m_cell)); }
#endif

private:
    JSCell* m_cell;
};

template <> class WriteBarrierBase<Unknown> {
public:
    void set(VM&, const JSCell* owner, JSValue value)
    {
        m_value = JSValue::encode(value);
        Heap::writeBarrier(owner, value);
    }

    void setWithoutWriteBarrier(JSValue value)
    {
        m_value = JSValue::encode(value);
    }

    JSValue get() const
    {
        return JSValue::decode(m_value);
    }
    void clear() { m_value = JSValue::encode(JSValue()); }
    void setUndefined() { m_value = JSValue::encode(jsUndefined()); }
    bool isNumber() const { return get().isNumber(); }
    bool isObject() const { return get().isObject(); }
    bool isNull() const { return get().isNull(); }
    bool isGetterSetter() const { return get().isGetterSetter(); }
    
    JSValue* slot()
    { 
        union {
            EncodedJSValue* v;
            JSValue* slot;
        } u;
        u.v = &m_value;
        return u.slot;
    }
    
    int32_t* tagPointer() { return &bitwise_cast<EncodedValueDescriptor*>(&m_value)->asBits.tag; }
    int32_t* payloadPointer() { return &bitwise_cast<EncodedValueDescriptor*>(&m_value)->asBits.payload; }
    
    typedef JSValue (WriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return get() ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    bool operator!() const { return !get(); } 
    
private:
    EncodedJSValue m_value;
};

template <typename T> class WriteBarrier : public WriteBarrierBase<T> {
public:
    WriteBarrier()
    {
        this->setWithoutWriteBarrier(0);
    }

    WriteBarrier(VM& vm, const JSCell* owner, T* value)
    {
        this->set(vm, owner, value);
    }

    enum MayBeNullTag { MayBeNull };
    WriteBarrier(VM& vm, const JSCell* owner, T* value, MayBeNullTag)
    {
        this->setMayBeNull(vm, owner, value);
    }
};

template <> class WriteBarrier<Unknown> : public WriteBarrierBase<Unknown> {
public:
    WriteBarrier()
    {
        this->setWithoutWriteBarrier(JSValue());
    }

    WriteBarrier(VM& vm, const JSCell* owner, JSValue value)
    {
        this->set(vm, owner, value);
    }
};

template <typename U, typename V> inline bool operator==(const WriteBarrierBase<U>& lhs, const WriteBarrierBase<V>& rhs)
{
    return lhs.get() == rhs.get();
}

// SlotVisitor functions

template<typename T> inline void SlotVisitor::append(WriteBarrierBase<T>* slot)
{
    internalAppend(*slot->slot());
}

ALWAYS_INLINE void SlotVisitor::appendValues(WriteBarrierBase<Unknown>* barriers, size_t count)
{
    append(barriers->slot(), count);
}

} // namespace JSC

#endif // WriteBarrier_h
