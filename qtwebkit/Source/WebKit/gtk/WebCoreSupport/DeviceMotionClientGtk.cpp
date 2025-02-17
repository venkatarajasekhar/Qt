/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#if ENABLE(DEVICE_ORIENTATION)
#include "DeviceMotionClientGtk.h"

#include "NotImplemented.h"

using namespace WebCore;

namespace WebKit {

DeviceMotionClientGtk::DeviceMotionClientGtk()
{
}

DeviceMotionClientGtk::~DeviceMotionClientGtk()
{
}

void DeviceMotionClientGtk::deviceMotionControllerDestroyed()
{
    delete this;
}

void DeviceMotionClientGtk::setController(DeviceMotionController* controller)
{
    m_controller = controller;
}

void DeviceMotionClientGtk::startUpdating()
{
    notImplemented();
}

void DeviceMotionClientGtk::stopUpdating()
{
    notImplemented();
}

DeviceMotionData* DeviceMotionClientGtk::lastMotion() const
{
    notImplemented();
    return 0;
}

} // namespece WebKit

#endif
