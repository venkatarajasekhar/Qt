/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SQL_DATABASE)

#include "SQLException.h"

namespace WebCore {

static struct SQLExceptionNameDescription {
    const char* const name;
    const char* const description;
} sqlExceptions[] = {
    { "UNKNOWN_ERR", "The operation failed for reasons unrelated to the database." },
    { "DATABASE_ERR", "The operation failed for some reason related to the database." },
    { "VERSION_ERR", "The actual database version did not match the expected version." },
    { "TOO_LARGE_ERR", "Data returned from the database is too large." },
    { "QUOTA_ERR", "Quota was exceeded." },
    { "SYNTAX_ERR", "Invalid or unauthorized statement; or the number of arguments did not match the number of ? placeholders." },
    { "CONSTRAINT_ERR", "A constraint was violated." },
    { "TIMEOUT_ERR", "A transaction lock could not be acquired in a reasonable time." }
};

bool SQLException::initializeDescription(ExceptionCode ec, ExceptionCodeDescription* description)
{
    if (ec < SQLExceptionOffset || ec > SQLExceptionMax)
        return false;

    description->typeName = "DOM SQL";
    description->code = ec - SQLExceptionOffset;
    description->type = SQLExceptionType;

    size_t tableSize = WTF_ARRAY_LENGTH(sqlExceptions);
    size_t tableIndex = ec - UNKNOWN_ERR;

    description->name = tableIndex < tableSize ? sqlExceptions[tableIndex].name : 0;
    description->description = tableIndex < tableSize ? sqlExceptions[tableIndex].description : 0;

    return true;
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)
