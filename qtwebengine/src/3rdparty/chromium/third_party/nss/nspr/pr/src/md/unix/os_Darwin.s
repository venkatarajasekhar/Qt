# -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef __i386__
#include "os_Darwin_x86.s"
#elif defined(__x86_64__)
#include "os_Darwin_x86_64.s"
#elif defined(__ppc__)
#include "os_Darwin_ppc.s"
#endif
