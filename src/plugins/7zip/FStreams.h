// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "7za/CPP/Common/MyCom.h"
#include "7za/CPP/7zip/IStream.h"

// Streams that feed 7za with data from disk.
//
// They implement the 7za stream interfaces directly instead of deriving from
// CInFileStream/COutFileStream: the bundled 7za is built with FString ==
// AString (USE_UNICODE_FSTRING is off), so its file layer converts char paths
// with the ANSI code page and is limited to MAX_PATH. Neither works for the
// UTF-8, potentially long paths of plugin interface 104.
//
// Every path handed to these classes is UTF-8 (as received from Salamander);
// it is opened through the W file API with the \\?\ prefix (splunicode.h).
// On a read/write error the user is offered Retry/Abort.

class CRetryableOutFileStream : public IOutStream,
                                public CMyUnknownImp
{
public:
    CRetryableOutFileStream(HWND hParentWnd);

    virtual ~CRetryableOutFileStream();

    // 'u8FileName' is a UTF-8 path coming from the Salamander interface
    bool Open(const char* u8FileName, DWORD creationDisposition);
    bool Close();

    bool SetMTime(const FILETIME* mTime);

    MY_UNKNOWN_IMP1(IOutStream)

    STDMETHOD(Write)
    (const void* data, UInt32 size, UInt32* processedSize);
    STDMETHOD(Seek)
    (Int64 offset, UInt32 seekOrigin, UInt64* newPosition);
    STDMETHOD(SetSize)
    (UInt64 newSize);

private:
    HANDLE Handle;
    HWND hParentWnd;
};

class CRetryableInFileStream : public IInStream,
                               public IStreamGetSize,
                               public CMyUnknownImp
{
public:
    CRetryableInFileStream(HWND hParentWnd);

    virtual ~CRetryableInFileStream();

    // 'u8FileName' is a UTF-8 path coming from the Salamander interface
    bool Open(const char* u8FileName);
    bool Close();

    MY_UNKNOWN_IMP2(IInStream, IStreamGetSize)

    STDMETHOD(Read)
    (void* data, UInt32 size, UInt32* processedSize);
    STDMETHOD(Seek)
    (Int64 offset, UInt32 seekOrigin, UInt64* newPosition);
    STDMETHOD(GetSize)
    (UInt64* size);

private:
    HANDLE Handle;
    HWND hParentWnd;
};
