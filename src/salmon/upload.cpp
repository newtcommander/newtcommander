// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

// Crash-report uploading is permanently disabled in Tandem Commander (feature 032):
// reports are only stored locally and the user may attach them to a GitHub issue.
// The former implementation posted the report archive over HTTP to the upstream
// vendor's server; that network code was removed together with the rebrand so the
// application cannot transmit crash data anywhere.

BOOL StartUploadThread(CUploadParams* params)
{
    if (params != NULL)
    {
        params->Result = FALSE;
        sprintf(params->ErrorMessage, "Crash-report uploading is disabled in Tandem Commander.");
    }
    return FALSE;
}

BOOL IsUploadThreadRunning()
{
    return FALSE;
}
