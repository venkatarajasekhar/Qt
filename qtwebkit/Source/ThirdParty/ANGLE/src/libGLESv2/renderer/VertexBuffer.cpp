#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer.cpp: Defines the abstract VertexBuffer class and VertexBufferInterface
// class with derivations, classes that perform graphics API agnostic vertex buffer operations.

#include "libGLESv2/renderer/VertexBuffer.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Context.h"

namespace rx
{

unsigned int VertexBuffer::mNextSerial = 1;

VertexBuffer::VertexBuffer()
{
    updateSerial();
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::updateSerial()
{
    mSerial = mNextSerial++;
}

unsigned int VertexBuffer::getSerial() const
{
    return mSerial;
}

VertexBufferInterface::VertexBufferInterface(rx::Renderer *renderer, bool dynamic) : mRenderer(renderer)
{
    mDynamic = dynamic;
    mWritePosition = 0;
    mReservedSpace = 0;

    mVertexBuffer = renderer->createVertexBuffer();
}

VertexBufferInterface::~VertexBufferInterface()
{
    delete mVertexBuffer;
}

unsigned int VertexBufferInterface::getSerial() const
{
    return mVertexBuffer->getSerial();
}

unsigned int VertexBufferInterface::getBufferSize() const
{
    return mVertexBuffer->getBufferSize();
}

bool VertexBufferInterface::setBufferSize(unsigned int size)
{
    if (mVertexBuffer->getBufferSize() == 0)
    {
        return mVertexBuffer->initialize(size, mDynamic);
    }
    else
    {
        return mVertexBuffer->setBufferSize(size);
    }
}

unsigned int VertexBufferInterface::getWritePosition() const
{
    return mWritePosition;
}

void VertexBufferInterface::setWritePosition(unsigned int writePosition)
{
    mWritePosition = writePosition;
}

bool VertexBufferInterface::discard()
{
    return mVertexBuffer->discard();
}

int VertexBufferInterface::storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count, GLsizei instances)
{
    if (!reserveSpace(mReservedSpace))
    {
        return -1;
    }
    mReservedSpace = 0;

    if (!mVertexBuffer->storeVertexAttributes(attrib, start, count, instances, mWritePosition))
    {
        return -1;
    }

    int oldWritePos = static_cast<int>(mWritePosition);
    mWritePosition += mVertexBuffer->getSpaceRequired(attrib, count, instances);

    return oldWritePos;
}

int VertexBufferInterface::storeRawData(const void* data, unsigned int size)
{
    if (!reserveSpace(mReservedSpace))
    {
        return -1;
    }
    mReservedSpace = 0;

    if (!mVertexBuffer->storeRawData(data, size, mWritePosition))
    {
        return -1;
    }

    int oldWritePos = static_cast<int>(mWritePosition);
    mWritePosition += size;

    return oldWritePos;
}

bool VertexBufferInterface::reserveVertexSpace(const gl::VertexAttribute &attribute, GLsizei count, GLsizei instances)
{
    unsigned int requiredSpace = mVertexBuffer->getSpaceRequired(attribute, count, instances);

    // Protect against integer overflow
    if (mReservedSpace + requiredSpace < mReservedSpace)
    {
         return false;
    }

    mReservedSpace += requiredSpace;
    return true;
}

bool VertexBufferInterface::reserveRawDataSpace(unsigned int size)
{
    // Protect against integer overflow
    if (mReservedSpace + size < mReservedSpace)
    {
         return false;
    }

    mReservedSpace += size;
    return true;
}

VertexBuffer* VertexBufferInterface::getVertexBuffer() const
{
    return mVertexBuffer;
}


StreamingVertexBufferInterface::StreamingVertexBufferInterface(rx::Renderer *renderer, std::size_t initialSize) : VertexBufferInterface(renderer, true)
{
    setBufferSize(initialSize);
}

StreamingVertexBufferInterface::~StreamingVertexBufferInterface()
{
}

bool StreamingVertexBufferInterface::reserveSpace(unsigned int size)
{
    bool result = true;
    unsigned int curBufferSize = getBufferSize();
    if (size > curBufferSize)
    {
        result = setBufferSize(std::max(size, 3 * curBufferSize / 2));
        setWritePosition(0);
    }
    else if (getWritePosition() + size > curBufferSize)
    {
        if (!discard())
        {
            return false;
        }
        setWritePosition(0);
    }

    return result;
}

StaticVertexBufferInterface::StaticVertexBufferInterface(rx::Renderer *renderer) : VertexBufferInterface(renderer, false)
{
}

StaticVertexBufferInterface::~StaticVertexBufferInterface()
{
}

int StaticVertexBufferInterface::lookupAttribute(const gl::VertexAttribute &attribute)
{
    for (unsigned int element = 0; element < mCache.size(); element++)
    {
        if (mCache[element].type == attribute.mType &&
            mCache[element].size == attribute.mSize &&
            mCache[element].stride == attribute.stride() &&
            mCache[element].normalized == attribute.mNormalized)
        {
            if (mCache[element].attributeOffset == attribute.mOffset % attribute.stride())
            {
                return mCache[element].streamOffset;
            }
        }
    }

    return -1;
}

bool StaticVertexBufferInterface::reserveSpace(unsigned int size)
{
    unsigned int curSize = getBufferSize();
    if (curSize == 0)
    {
        setBufferSize(size);
        return true;
    }
    else if (curSize >= size)
    {
        return true;
    }
    else
    {
        UNREACHABLE();   // Static vertex buffers can't be resized
        return false;
    }
}

int StaticVertexBufferInterface::storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count, GLsizei instances)
{
    int attributeOffset = attrib.mOffset % attrib.stride();
    VertexElement element = { attrib.mType, attrib.mSize, attrib.stride(), attrib.mNormalized, attributeOffset, getWritePosition() };
    mCache.push_back(element);

    return VertexBufferInterface::storeVertexAttributes(attrib, start, count, instances);
}

}
