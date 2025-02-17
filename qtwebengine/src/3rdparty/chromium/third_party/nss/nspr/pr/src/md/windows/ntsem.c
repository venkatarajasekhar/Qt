/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * NT-specific semaphore handling code.
 *
 */


#include "primpl.h"


void 
_PR_MD_NEW_SEM(_MDSemaphore *md, PRUintn value)
{
    md->sem = CreateSemaphore(NULL, value, 0x7fffffff, NULL);
}

void 
_PR_MD_DESTROY_SEM(_MDSemaphore *md)
{
    CloseHandle(md->sem);
}

PRStatus 
_PR_MD_TIMED_WAIT_SEM(_MDSemaphore *md, PRIntervalTime ticks)
{
    int rv;

    rv = WaitForSingleObject(md->sem, PR_IntervalToMilliseconds(ticks));

    if (rv == WAIT_OBJECT_0)
        return PR_SUCCESS;
    else
        return PR_FAILURE;
}

PRStatus 
_PR_MD_WAIT_SEM(_MDSemaphore *md)
{
    return _PR_MD_TIMED_WAIT_SEM(md, PR_INTERVAL_NO_TIMEOUT);
}

void 
_PR_MD_POST_SEM(_MDSemaphore *md)
{
    ReleaseSemaphore(md->sem, 1, NULL);
}
