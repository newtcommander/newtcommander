// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "sftputils.h"
#include "keyload.h"
#include "dialogs.h"

char ConnectPlainPassword[512] = "";
char ConnectPlainPassphrase[512] = "";

// ---------------------------------------------------------------------------
// UTF-8 dialog text helpers (feature 010)
// ---------------------------------------------------------------------------

// Sets a dialog control's text from a UTF-8 string (interface 104) via the W
// API so names outside the ACP display correctly; falls back to the A call on
// conversion failure (invalid UTF-8 is still shown, never dropped).
void SetDlgItemTextU8(HWND hwnd, int id, const char* text)
{
    WCHAR* w = SplU8ToWAlloc(text);
    if (w != NULL)
        SetDlgItemTextW(hwnd, id, w);
    else
        SetDlgItemTextA(hwnd, id, text != NULL ? text : "");
    free(w);
}

// Reads a dialog control's text as UTF-8 via the W API (the A call would
// convert through the ACP and mangle names). Returns the length in bytes
// without the terminator; falls back to the A call on conversion failure.
int GetDlgItemTextU8(HWND hwnd, int id, char* buf, int bufSize)
{
    if (buf == NULL || bufSize <= 0)
        return 0;
    buf[0] = 0;
    HWND ctrl = GetDlgItem(hwnd, id);
    if (ctrl == NULL)
        return 0;
    int wchars = GetWindowTextLengthW(ctrl);
    if (wchars <= 0)
        return 0;
    int len = 0;
    WCHAR* w = (WCHAR*)malloc((wchars + 1) * sizeof(WCHAR));
    if (w != NULL)
    {
        if (GetWindowTextW(ctrl, w, wchars + 1) > 0)
        {
            len = SplWToU8(w, buf, bufSize);
            if (len > 0)
                len--; // exclude the terminator
        }
        free(w);
    }
    if (len == 0) // OOM or conversion failure: A fallback (never drop text)
        len = GetDlgItemTextA(hwnd, id, buf, bufSize);
    return len;
}

// Adds a UTF-8 string to a list box via the W API; A fallback on invalid UTF-8.
int ListBoxAddStringU8(HWND lb, const char* text)
{
    WCHAR* w = SplU8ToWAlloc(text);
    if (w == NULL)
        return (int)SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)text);
    int idx = (int)SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)w);
    free(w);
    return idx;
}

// ---------------------------------------------------------------------------
// password / passphrase prompt
// ---------------------------------------------------------------------------

struct CPasswordPromptData
{
    const char* Prompt;
    char* Out;
    int OutSize;
};

static INT_PTR CALLBACK PasswordPromptProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CPasswordPromptData* d = (CPasswordPromptData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_INITDIALOG:
        d = (CPasswordPromptData*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
        SetDlgItemTextU8(hwnd, IDT_PROMPTTEXT, d->Prompt); // contains the user name
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        SetFocus(GetDlgItem(hwnd, IDE_PROMPTPASSWORD));
        return FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemTextA(hwnd, IDE_PROMPTPASSWORD, d->Out, d->OutSize);
            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL ShowPasswordPrompt(HWND parent, const char* promptText, char* out, int outSize)
{
    CPasswordPromptData d;
    d.Prompt = promptText;
    d.Out = out;
    d.OutSize = outSize;
    out[0] = 0;
    return DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_PASSWORD), parent, PasswordPromptProc, (LPARAM)&d) == IDOK;
}

// ---------------------------------------------------------------------------
// host-key verification (TOFU)
// ---------------------------------------------------------------------------

struct CHostKeyData
{
    BOOL Changed;
    const char* Host;
    int Port;
    const char* KeyType;
    const char* Fingerprint;
    const char* StoredFingerprint;
};

static INT_PTR CALLBACK HostKeyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CHostKeyData* d = (CHostKeyData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        d = (CHostKeyData*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
        char hostPort[320];
        _snprintf_s(hostPort, _TRUNCATE, "%s:%d", d->Host, d->Port);
        char text[1400];
        if (d->Changed)
        {
            SetWindowTextA(hwnd, LoadStr(IDS_HOSTKEY_CHANGEDTITLE));
            _snprintf_s(text, _TRUNCATE, LoadStr(IDS_HOSTKEY_CHANGEDTEXT),
                        d->KeyType, hostPort,
                        d->StoredFingerprint != NULL ? d->StoredFingerprint : "?",
                        d->Fingerprint);
            SetDlgItemTextA(hwnd, IDB_HOSTKEY_TRUST, LoadStr(IDS_HOSTKEY_ACCEPTNEW));
        }
        else
        {
            SetWindowTextA(hwnd, LoadStr(IDS_HOSTKEY_NEWTITLE));
            _snprintf_s(text, _TRUNCATE, LoadStr(IDS_HOSTKEY_NEWTEXT), hostPort, d->KeyType, d->Fingerprint);
        }
        SetDlgItemTextU8(hwnd, IDT_HOSTKEY_TEXT, text); // contains the host name
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDB_HOSTKEY_TRUST:
            EndDialog(hwnd, IDB_HOSTKEY_TRUST);
            return TRUE;
        case IDB_HOSTKEY_ONCE:
            EndDialog(hwnd, IDB_HOSTKEY_ONCE);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

int ShowHostKeyDialog(HWND parent, BOOL changed, const char* host, int port,
                      const char* keyType, const char* fingerprint, const char* storedFingerprint)
{
    CHostKeyData d;
    d.Changed = changed;
    d.Host = host;
    d.Port = port;
    d.KeyType = keyType;
    d.Fingerprint = fingerprint;
    d.StoredFingerprint = storedFingerprint;
    if (parent == NULL)
        parent = SalamanderGeneral->GetMsgBoxParent();
    return (int)DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_HOSTKEY), parent, HostKeyProc, (LPARAM)&d);
}

// ---------------------------------------------------------------------------
// resume prompt
// ---------------------------------------------------------------------------

int ShowResumePrompt(HWND parent, const char* fileName)
{
    char msg[1200];
    _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_RESUME_OVERWRITE), fileName);
    // Yes = resume, No = overwrite, Cancel = skip
    return SalamanderGeneral->SalMessageBox(parent, msg, LoadStr(IDS_RESUME_TITLE),
                                            MB_YESNOCANCEL | MB_ICONQUESTION);
}

// ---------------------------------------------------------------------------
// rename
// ---------------------------------------------------------------------------

struct CRenameData
{
    const char* Prompt;
    char* Name;
};

static INT_PTR CALLBACK RenameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CRenameData* d = (CRenameData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_INITDIALOG:
        d = (CRenameData*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
        SetDlgItemTextU8(hwnd, IDT_RENAMEPROMPT, d->Prompt);
        SetDlgItemTextU8(hwnd, IDE_RENAMENAME, d->Name);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        SetFocus(GetDlgItem(hwnd, IDE_RENAMENAME));
        return FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemTextU8(hwnd, IDE_RENAMENAME, d->Name, MAX_PATH);
            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL ShowRenameDialog(HWND parent, const char* prompt, char* name)
{
    CRenameData d;
    d.Prompt = prompt;
    d.Name = name;
    return DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_RENAME), parent, RenameProc, (LPARAM)&d) == IDOK;
}

// ---------------------------------------------------------------------------
// symlink
// ---------------------------------------------------------------------------

struct CSymlinkData
{
    char* Name;
    char* Target;
};

static INT_PTR CALLBACK SymlinkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CSymlinkData* d = (CSymlinkData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_INITDIALOG:
        d = (CSymlinkData*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
        SetDlgItemTextU8(hwnd, IDE_SYMLINKNAME, d->Name);
        SetDlgItemTextU8(hwnd, IDE_SYMLINKTARGET, d->Target);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemTextU8(hwnd, IDE_SYMLINKNAME, d->Name, MAX_PATH);
            GetDlgItemTextU8(hwnd, IDE_SYMLINKTARGET, d->Target, MAX_PATH);
            if (d->Name[0] == 0 || d->Target[0] == 0)
                return TRUE;
            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL ShowSymlinkDialog(HWND parent, char* name, char* target)
{
    CSymlinkData d;
    d.Name = name;
    d.Target = target;
    return DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_SYMLINK), parent, SymlinkProc, (LPARAM)&d) == IDOK;
}

// ---------------------------------------------------------------------------
// chmod
// ---------------------------------------------------------------------------

struct CChmodData
{
    const char* Label;
    BOOL Multiple;
    unsigned long Mode;
    BOOL Recurse;
    BOOL SetTime;
    __int64 Mtime;
};

static void ChmodModeToControls(HWND hwnd, unsigned long mode)
{
    CheckDlgButton(hwnd, IDC_UR, (mode & 0400) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_UW, (mode & 0200) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_UX, (mode & 0100) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_GR, (mode & 0040) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_GW, (mode & 0020) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_GX, (mode & 0010) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_OR, (mode & 0004) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_OW, (mode & 0002) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_OX, (mode & 0001) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_SETUID, (mode & 04000) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_SETGID, (mode & 02000) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_STICKY, (mode & 01000) ? BST_CHECKED : BST_UNCHECKED);
}

static unsigned long ChmodControlsToMode(HWND hwnd)
{
    unsigned long m = 0;
    if (IsDlgButtonChecked(hwnd, IDC_UR))
        m |= 0400;
    if (IsDlgButtonChecked(hwnd, IDC_UW))
        m |= 0200;
    if (IsDlgButtonChecked(hwnd, IDC_UX))
        m |= 0100;
    if (IsDlgButtonChecked(hwnd, IDC_GR))
        m |= 0040;
    if (IsDlgButtonChecked(hwnd, IDC_GW))
        m |= 0020;
    if (IsDlgButtonChecked(hwnd, IDC_GX))
        m |= 0010;
    if (IsDlgButtonChecked(hwnd, IDC_OR))
        m |= 0004;
    if (IsDlgButtonChecked(hwnd, IDC_OW))
        m |= 0002;
    if (IsDlgButtonChecked(hwnd, IDC_OX))
        m |= 0001;
    if (IsDlgButtonChecked(hwnd, IDC_SETUID))
        m |= 04000;
    if (IsDlgButtonChecked(hwnd, IDC_SETGID))
        m |= 02000;
    if (IsDlgButtonChecked(hwnd, IDC_STICKY))
        m |= 01000;
    return m;
}

static INT_PTR CALLBACK ChmodProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CChmodData* d = (CChmodData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        d = (CChmodData*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
        SetDlgItemTextU8(hwnd, IDT_CHMODTARGET, d->Label); // remote file name
        ChmodModeToControls(hwnd, d->Mode);
        char octal[8];
        FormatOctalMode(d->Mode, octal);
        SetDlgItemTextA(hwnd, IDE_OCTAL, octal);
        EnableWindow(GetDlgItem(hwnd, IDE_MTIME), FALSE);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_UR:
        case IDC_UW:
        case IDC_UX:
        case IDC_GR:
        case IDC_GW:
        case IDC_GX:
        case IDC_OR:
        case IDC_OW:
        case IDC_OX:
        case IDC_SETUID:
        case IDC_SETGID:
        case IDC_STICKY:
        {
            unsigned long m = ChmodControlsToMode(hwnd);
            char octal[8];
            FormatOctalMode(m, octal);
            SetDlgItemTextA(hwnd, IDE_OCTAL, octal);
            return TRUE;
        }
        case IDE_OCTAL:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                char txt[16];
                GetDlgItemTextA(hwnd, IDE_OCTAL, txt, sizeof(txt));
                unsigned long m;
                if (ParseOctalMode(txt, &m))
                    ChmodModeToControls(hwnd, m);
            }
            return TRUE;
        case IDC_SETTIME:
            EnableWindow(GetDlgItem(hwnd, IDE_MTIME), IsDlgButtonChecked(hwnd, IDC_SETTIME));
            return TRUE;
        case IDOK:
        {
            char txt[16];
            GetDlgItemTextA(hwnd, IDE_OCTAL, txt, sizeof(txt));
            unsigned long m;
            if (ParseOctalMode(txt, &m))
                d->Mode = m;
            else
                d->Mode = ChmodControlsToMode(hwnd);
            d->Recurse = IsDlgButtonChecked(hwnd, IDC_RECURSE);
            d->SetTime = IsDlgButtonChecked(hwnd, IDC_SETTIME);
            if (d->SetTime)
            {
                char mt[64];
                GetDlgItemTextA(hwnd, IDE_MTIME, mt, sizeof(mt));
                // accept a Unix epoch seconds value; empty -> now
                if (mt[0] != 0)
                    d->Mtime = _atoi64(mt);
                else
                {
                    FILETIME ft;
                    GetSystemTimeAsFileTime(&ft);
                    d->Mtime = FileTimeToUnixTime(&ft);
                }
            }
            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL ShowChmodDialog(HWND parent, const char* targetLabel, BOOL multiple,
                     unsigned long* mode, BOOL* recurse, BOOL* setTime, __int64* mtime)
{
    CChmodData d;
    d.Label = targetLabel;
    d.Multiple = multiple;
    d.Mode = *mode;
    d.Recurse = FALSE;
    d.SetTime = FALSE;
    d.Mtime = 0;
    if (DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_CHMOD), parent, ChmodProc, (LPARAM)&d) != IDOK)
        return FALSE;
    *mode = d.Mode;
    *recurse = d.Recurse;
    *setTime = d.SetTime;
    *mtime = d.Mtime;
    return TRUE;
}

// ---------------------------------------------------------------------------
// configuration
// ---------------------------------------------------------------------------

static INT_PTR CALLBACK ConfigProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        SetDlgItemInt(hwnd, IDE_CONNECTTIMEOUT, Config.ConnectTimeout, FALSE);
        SetDlgItemInt(hwnd, IDE_OPTIMEOUT, Config.OperationTimeout, FALSE);
        SetDlgItemInt(hwnd, IDE_KEEPALIVE, Config.KeepAliveSendEvery, FALSE);
        SetDlgItemInt(hwnd, IDE_KEEPALIVESTOP, Config.KeepAliveStopAfter, FALSE);
        SetDlgItemInt(hwnd, IDE_CONNRETRIES, Config.ConnectRetries, FALSE);
        SetDlgItemInt(hwnd, IDE_RETRYDELAY, Config.RetryDelay, FALSE);
        SetDlgItemInt(hwnd, IDE_RESUMEOVERLAP, Config.ResumeOverlap, FALSE);
        SetDlgItemInt(hwnd, IDE_RESUMEMINSIZE, Config.ResumeMinFileSize, FALSE);
        CheckRadioButton(hwnd, IDC_COLVIEW_RIGHTS, IDC_COLVIEW_ATTR,
                         Config.ColumnView == cvUnixRights ? IDC_COLVIEW_RIGHTS : IDC_COLVIEW_ATTR);
        CheckDlgButton(hwnd, IDC_SHOWOCTAL, Config.ShowOctal ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_ENABLELOG, Config.EnableLogging ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemInt(hwnd, IDE_LOGMAXSIZE, Config.LogMaxSize, FALSE);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            Config.ConnectTimeout = GetDlgItemInt(hwnd, IDE_CONNECTTIMEOUT, NULL, FALSE);
            Config.OperationTimeout = GetDlgItemInt(hwnd, IDE_OPTIMEOUT, NULL, FALSE);
            Config.KeepAliveSendEvery = GetDlgItemInt(hwnd, IDE_KEEPALIVE, NULL, FALSE);
            Config.KeepAliveStopAfter = GetDlgItemInt(hwnd, IDE_KEEPALIVESTOP, NULL, FALSE);
            Config.ConnectRetries = GetDlgItemInt(hwnd, IDE_CONNRETRIES, NULL, FALSE);
            Config.RetryDelay = GetDlgItemInt(hwnd, IDE_RETRYDELAY, NULL, FALSE);
            Config.ResumeOverlap = GetDlgItemInt(hwnd, IDE_RESUMEOVERLAP, NULL, FALSE);
            Config.ResumeMinFileSize = GetDlgItemInt(hwnd, IDE_RESUMEMINSIZE, NULL, FALSE);
            Config.ColumnView = IsDlgButtonChecked(hwnd, IDC_COLVIEW_RIGHTS) ? cvUnixRights : cvAttributes;
            Config.ShowOctal = IsDlgButtonChecked(hwnd, IDC_SHOWOCTAL);
            Config.EnableLogging = IsDlgButtonChecked(hwnd, IDC_ENABLELOG);
            Config.LogMaxSize = GetDlgItemInt(hwnd, IDE_LOGMAXSIZE, NULL, FALSE);
            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowConfigDialog(HWND parent)
{
    // IDD_CONFIG is a child-style template; host it in a simple modal frame by
    // temporarily giving it a caption via DS_MODALFRAME at runtime is awkward,
    // so we reuse it directly (it works as a modal dialog under DialogBox).
    DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_CONFIG), parent, ConfigProc, 0);
}

// ---------------------------------------------------------------------------
// connect / bookmarks
// ---------------------------------------------------------------------------

struct CConnectData
{
    BOOL OrganizeMode;
    CSFTPServer* Result;
    HWND Hwnd;
};

static void ConnectSetAuthMode(HWND hwnd, int authMethod)
{
    BOOL pwd = (authMethod == saPassword);
    EnableWindow(GetDlgItem(hwnd, IDE_PASSWORD), pwd);
    EnableWindow(GetDlgItem(hwnd, IDC_SAVEPASSWORD), pwd);
    EnableWindow(GetDlgItem(hwnd, IDE_KEYFILE), !pwd);
    EnableWindow(GetDlgItem(hwnd, IDB_BROWSEKEY), !pwd);
    EnableWindow(GetDlgItem(hwnd, IDE_PASSPHRASE), !pwd);
    EnableWindow(GetDlgItem(hwnd, IDC_SAVEPASSPHRASE), !pwd);
}

static void ConnectLoadServerToFields(HWND hwnd, const CSFTPServer* s)
{
    SetDlgItemTextU8(hwnd, IDE_HOSTADDRESS, s->Address != NULL ? s->Address : "");
    SetDlgItemInt(hwnd, IDE_PORT, s->Port > 0 ? s->Port : SFTP_DEFAULT_PORT, FALSE);
    SetDlgItemTextU8(hwnd, IDE_USERNAME, s->UserName != NULL ? s->UserName : "");
    CheckRadioButton(hwnd, IDC_AUTHPASSWORD, IDC_AUTHKEY,
                     s->AuthMethod == saPrivateKey ? IDC_AUTHKEY : IDC_AUTHPASSWORD);
    SetDlgItemTextA(hwnd, IDE_PASSWORD, "");
    CheckDlgButton(hwnd, IDC_SAVEPASSWORD, s->SavePassword ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextU8(hwnd, IDE_KEYFILE, s->KeyFile != NULL ? s->KeyFile : "");
    SetDlgItemTextA(hwnd, IDE_PASSPHRASE, "");
    CheckDlgButton(hwnd, IDC_SAVEPASSPHRASE, s->SavePassphrase ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextU8(hwnd, IDE_INITIALPATH, s->InitialPath != NULL ? s->InitialPath : "");
    ConnectSetAuthMode(hwnd, s->AuthMethod);
}

// reads dialog fields into 's'; encrypts typed secrets. When 'forConnect', also
// copies the plaintext into ConnectPlainPassword/Passphrase for the immediate
// connect (so a New/Rename-bookmark action never leaves a stale password that a
// later connect to a different server could pick up).
static BOOL ConnectReadFields(HWND hwnd, CSFTPServer* s, const CSFTPServer* selectedBookmark, BOOL forConnect)
{
    char host[256], user[256], keyfile[MAX_PATH], initpath[1024];
    GetDlgItemTextU8(hwnd, IDE_HOSTADDRESS, host, sizeof(host));
    GetDlgItemTextU8(hwnd, IDE_USERNAME, user, sizeof(user));
    GetDlgItemTextU8(hwnd, IDE_KEYFILE, keyfile, sizeof(keyfile));
    GetDlgItemTextU8(hwnd, IDE_INITIALPATH, initpath, sizeof(initpath));
    int port = GetDlgItemInt(hwnd, IDE_PORT, NULL, FALSE);
    if (host[0] == 0)
    {
        SalamanderGeneral->SalMessageBox(hwnd, LoadStr(IDS_HOSTNAMEMISSING),
                                         LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (port <= 0 || port > 65535)
    {
        SalamanderGeneral->SalMessageBox(hwnd, LoadStr(IDS_INVALIDPORT),
                                         LoadStr(IDS_SFTPERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    int authMethod = IsDlgButtonChecked(hwnd, IDC_AUTHKEY) ? saPrivateKey : saPassword;
    s->Set(NULL, host, port, user);
    s->AuthMethod = authMethod;
    s->SetString(&s->KeyFile, keyfile[0] ? keyfile : NULL);
    s->SetString(&s->InitialPath, initpath[0] ? initpath : NULL);
    s->SavePassword = IsDlgButtonChecked(hwnd, IDC_SAVEPASSWORD);
    s->SavePassphrase = IsDlgButtonChecked(hwnd, IDC_SAVEPASSPHRASE);

    if (forConnect)
    {
        ConnectPlainPassword[0] = 0;
        ConnectPlainPassphrase[0] = 0;
    }

    CSalamanderPasswordManagerAbstract* pm = SalamanderGeneral->GetSalamanderPasswordManager();

    if (authMethod == saPassword)
    {
        char pwd[512];
        GetDlgItemTextA(hwnd, IDE_PASSWORD, pwd, sizeof(pwd));
        if (pwd[0] != 0)
        {
            if (forConnect)
                lstrcpynA(ConnectPlainPassword, pwd, sizeof(ConnectPlainPassword));
            if (s->SavePassword && pm != NULL)
            {
                BOOL enc = pm->IsUsingMasterPassword() && pm->IsMasterPasswordSet();
                if (pm->IsUsingMasterPassword() && !pm->IsMasterPasswordSet())
                    enc = pm->AskForMasterPassword(hwnd);
                BYTE* blob = NULL;
                int blobSize = 0;
                if (pm->EncryptPassword(pwd, &blob, &blobSize, enc) && blob != NULL)
                {
                    s->SetBlob(&s->EncryptedPassword, &s->EncryptedPasswordSize, blob, blobSize);
                    SalamanderGeneral->Free(blob);
                }
            }
            SecureZeroMemory(pwd, sizeof(pwd));
        }
        else if (s->SavePassword && selectedBookmark != NULL && selectedBookmark->SavePassword &&
                 selectedBookmark->EncryptedPassword != NULL)
        {
            // feature 017: reuse the stored blob when the password field is left
            // blank AND "Save password" is still on. Gating on s->SavePassword
            // (the current checkbox) means unchecking it clears the saved secret
            // (FR-007) instead of silently keeping it.
            s->SetBlob(&s->EncryptedPassword, &s->EncryptedPasswordSize,
                       selectedBookmark->EncryptedPassword, selectedBookmark->EncryptedPasswordSize);
        }
    }
    else // key
    {
        char pass[512];
        GetDlgItemTextA(hwnd, IDE_PASSPHRASE, pass, sizeof(pass));
        if (pass[0] != 0)
        {
            if (forConnect)
                lstrcpynA(ConnectPlainPassphrase, pass, sizeof(ConnectPlainPassphrase));
            if (s->SavePassphrase && pm != NULL)
            {
                BOOL enc = pm->IsUsingMasterPassword() && pm->IsMasterPasswordSet();
                if (pm->IsUsingMasterPassword() && !pm->IsMasterPasswordSet())
                    enc = pm->AskForMasterPassword(hwnd);
                BYTE* blob = NULL;
                int blobSize = 0;
                if (pm->EncryptPassword(pass, &blob, &blobSize, enc) && blob != NULL)
                {
                    s->SetBlob(&s->EncryptedPassphrase, &s->EncryptedPassphraseSize, blob, blobSize);
                    SalamanderGeneral->Free(blob);
                }
            }
            SecureZeroMemory(pass, sizeof(pass));
        }
        else if (s->SavePassphrase && selectedBookmark != NULL && selectedBookmark->SavePassphrase &&
                 selectedBookmark->EncryptedPassphrase != NULL)
        {
            // feature 017: mirror the password reuse/clear rule for the passphrase
            s->SetBlob(&s->EncryptedPassphrase, &s->EncryptedPassphraseSize,
                       selectedBookmark->EncryptedPassphrase, selectedBookmark->EncryptedPassphraseSize);
        }
    }
    return TRUE;
}

// feature 017: item-data convention for the bookmark list.
//   QC_ITEM (-1) = the "Quick Connect" pseudo-row; >= 0 = index into Config.Bookmarks.
#define QC_ITEM (-1)

static void ConnectFillBookmarkList(HWND hwnd)
{
    HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
    SendMessage(lb, LB_RESETCONTENT, 0, 0);
    // feature 017 (U3): a visible "Quick Connect" row makes the scratch entry an
    // explicit, selectable target instead of an invisible "no selection" mode
    int qc = ListBoxAddStringU8(lb, LoadStr(IDS_QUICKCONNECT));
    SendMessage(lb, LB_SETITEMDATA, qc, (LPARAM)QC_ITEM);
    for (int i = 0; i < Config.Bookmarks.Count; i++)
    {
        CSFTPServer* s = Config.Bookmarks[i];
        const char* name = (s->ItemName != NULL && s->ItemName[0]) ? s->ItemName : (s->Address != NULL ? s->Address : "(unnamed)");
        int idx = ListBoxAddStringU8(lb, name);
        SendMessage(lb, LB_SETITEMDATA, idx, i);
    }
}

// feature 017: the entry currently selected in the list. Returns the
// Config.Bookmarks index (>= 0), QC_ITEM for Quick Connect, or LB_ERR for none.
static int ConnectSelectedItemData(HWND hwnd)
{
    HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
    int sel = (int)SendMessage(lb, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR)
        return LB_ERR;
    return (int)SendMessage(lb, LB_GETITEMDATA, sel, 0);
}

// feature 017: resolve the item data to the persistent entry it edits.
static CSFTPServer* ConnectEntryForItemData(int itemData)
{
    if (itemData == QC_ITEM)
        return &Config.QuickConnect;
    if (itemData >= 0 && itemData < Config.Bookmarks.Count)
        return Config.Bookmarks[itemData];
    return NULL;
}

// feature 017 (P3/U1): commit the current dialog fields into a persistent entry,
// preserving its bookmark name (ConnectReadFields nulls ItemName via Set). The
// entry's own stored blob is offered as the reuse fallback so an unchanged
// password field keeps the saved secret. Returns FALSE on validation error.
static BOOL ConnectCommitToEntry(HWND hwnd, CSFTPServer* entry, BOOL forConnect)
{
    CSFTPServer tmp; // separate object: avoids self-aliasing in SetBlob reuse
    if (!ConnectReadFields(hwnd, &tmp, entry, forConnect))
        return FALSE;
    char* savedName = (entry->ItemName != NULL) ? _strdup(entry->ItemName) : NULL;
    BOOL ok = entry->CopyFrom(&tmp);
    entry->SetString(&entry->ItemName, savedName); // restore name (CopyFrom took tmp's NULL)
    if (savedName != NULL)
        free(savedName);
    return ok;
}

// feature 017: select the list row whose item data matches (falls back to the
// Quick Connect row at index 0).
static void ConnectSelectItemData(HWND hwnd, int itemData)
{
    HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
    int n = (int)SendMessage(lb, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < n; i++)
        if ((int)SendMessage(lb, LB_GETITEMDATA, i, 0) == itemData)
        {
            SendMessage(lb, LB_SETCURSEL, i, 0);
            return;
        }
    SendMessage(lb, LB_SETCURSEL, 0, 0);
}

// feature 017 (U4): Rename/Delete apply to real bookmarks only; Quick Connect
// cannot be renamed or removed. Duplicate works on either.
static void ConnectUpdateButtons(HWND hwnd)
{
    int data = ConnectSelectedItemData(hwnd);
    BOOL isBookmark = (data >= 0);
    EnableWindow(GetDlgItem(hwnd, IDB_RENAMEBOOKMARK), isBookmark);
    EnableWindow(GetDlgItem(hwnd, IDB_REMOVEBOOKMARK), isBookmark);
    EnableWindow(GetDlgItem(hwnd, IDB_COPYBOOKMARK), data != LB_ERR);
}

static void BrowseForKey(HWND hwnd)
{
    // the key-file path is UTF-8 (interface 104) -> use the W common dialog (feature 010)
    char u8file[MAX_PATH];
    WCHAR file[MAX_PATH];
    GetDlgItemTextU8(hwnd, IDE_KEYFILE, u8file, sizeof(u8file));
    if (SplU8ToW(u8file, file, MAX_PATH) == 0)
        file[0] = 0;
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"All files\0*.*\0Private keys\0*.pem;*.ppk;*.key;id_*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn))
        SetDlgItemTextW(hwnd, IDE_KEYFILE, file);
}

static INT_PTR CALLBACK ConnectProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CConnectData* d = (CConnectData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        d = (CConnectData*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
        d->Hwnd = hwnd;
        ConnectFillBookmarkList(hwnd);
        if (d->OrganizeMode)
        {
            SetWindowTextA(hwnd, LoadStr(IDS_MENU_ORGANIZEBOOKMARKS));
            SetDlgItemTextA(hwnd, IDB_CONNECT, "Close");
        }
        // feature 017 (U3): seed from the last-used entry and select its row.
        // Quick Connect is item 0; a saved bookmark selects its own row.
        if (Config.LastBookmark > 0 && Config.LastBookmark <= Config.Bookmarks.Count)
        {
            int bi = Config.LastBookmark - 1;
            ConnectLoadServerToFields(hwnd, Config.Bookmarks[bi]);
            ConnectSelectItemData(hwnd, bi);
        }
        else
        {
            ConnectLoadServerToFields(hwnd, &Config.QuickConnect);
            ConnectSelectItemData(hwnd, QC_ITEM);
        }
        ConnectUpdateButtons(hwnd);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_AUTHPASSWORD:
            ConnectSetAuthMode(hwnd, saPassword);
            return TRUE;
        case IDC_AUTHKEY:
            ConnectSetAuthMode(hwnd, saPrivateKey);
            return TRUE;
        case IDB_BROWSEKEY:
            BrowseForKey(hwnd);
            return TRUE;
        case IDL_BOOKMARKS:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                // feature 017: load the selected entry (Quick Connect or a
                // bookmark) into the fields and update which buttons apply
                int data = ConnectSelectedItemData(hwnd);
                CSFTPServer* entry = ConnectEntryForItemData(data);
                if (entry != NULL)
                    ConnectLoadServerToFields(hwnd, entry);
                ConnectUpdateButtons(hwnd);
            }
            else if (HIWORD(wParam) == LBN_DBLCLK)
            {
                // feature 017 (U5): double-click a row = Connect
                if (!d->OrganizeMode)
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDB_CONNECT, 0), 0);
            }
            return TRUE;
        case IDB_NEWBOOKMARK:
        {
            // feature 017: "Save as new bookmark" - snapshot the current fields
            // (incl. a just-typed+encrypted password) under a new name
            char name[MAX_PATH] = "";
            if (ShowRenameDialog(hwnd, LoadStr(IDS_BOOKMARKNAME), name) && name[0] != 0)
            {
                CSFTPServer* s = new CSFTPServer;
                if (s != NULL && ConnectReadFields(hwnd, s, NULL, FALSE)) // new bookmark: don't touch connect globals
                {
                    s->SetString(&s->ItemName, name);
                    Config.Bookmarks.Add(s);
                    ConnectFillBookmarkList(hwnd);
                    ConnectSelectItemData(hwnd, Config.Bookmarks.Count - 1);
                    ConnectUpdateButtons(hwnd);
                }
                else
                    delete s;
            }
            return TRUE;
        }
        case IDB_COPYBOOKMARK:
        {
            // feature 017 (U2): Duplicate - copy the selected entry's STORED data
            // (not the possibly-edited fields) into a new named bookmark. Wires
            // the previously dead IDB_COPYBOOKMARK control.
            int data = ConnectSelectedItemData(hwnd);
            CSFTPServer* src = ConnectEntryForItemData(data);
            if (src == NULL)
                return TRUE;
            char name[MAX_PATH];
            lstrcpynA(name, src->ItemName != NULL ? src->ItemName : (src->Address != NULL ? src->Address : ""), sizeof(name));
            if (ShowRenameDialog(hwnd, LoadStr(IDS_BOOKMARKNAME), name) && name[0] != 0)
            {
                CSFTPServer* s = new CSFTPServer;
                if (s != NULL && s->CopyFrom(src))
                {
                    s->SetString(&s->ItemName, name);
                    Config.Bookmarks.Add(s);
                    ConnectFillBookmarkList(hwnd);
                    ConnectSelectItemData(hwnd, Config.Bookmarks.Count - 1);
                    ConnectLoadServerToFields(hwnd, s);
                    ConnectUpdateButtons(hwnd);
                }
                else
                    delete s;
            }
            return TRUE;
        }
        case IDB_SAVEBOOKMARK:
        {
            // feature 017 (U1/P3): commit the current fields (incl. a typed and
            // encrypted password) into the selected entry - the explicit "save
            // edits back to this bookmark/quick-connect" action that was missing
            int data = ConnectSelectedItemData(hwnd);
            CSFTPServer* entry = ConnectEntryForItemData(data);
            if (entry != NULL && ConnectCommitToEntry(hwnd, entry, FALSE))
            {
                ConnectFillBookmarkList(hwnd);
                ConnectSelectItemData(hwnd, data);
                ConnectUpdateButtons(hwnd);
                SalamanderGeneral->SalMessageBox(hwnd, LoadStr(IDS_BOOKMARKSAVED),
                                                 LoadStr(IDS_SFTPPLUGINTITLE), MB_OK | MB_ICONINFORMATION);
            }
            return TRUE;
        }
        case IDB_RENAMEBOOKMARK:
        {
            int data = ConnectSelectedItemData(hwnd);
            if (data < 0)
                return TRUE; // Quick Connect cannot be renamed
            CSFTPServer* s = Config.Bookmarks[data];
            char name[MAX_PATH];
            lstrcpynA(name, s->ItemName != NULL ? s->ItemName : "", sizeof(name));
            if (ShowRenameDialog(hwnd, LoadStr(IDS_BOOKMARKNAME), name) && name[0] != 0)
            {
                s->SetString(&s->ItemName, name);
                ConnectFillBookmarkList(hwnd);
                ConnectSelectItemData(hwnd, data);
                ConnectUpdateButtons(hwnd);
            }
            return TRUE;
        }
        case IDB_REMOVEBOOKMARK:
        {
            // feature 017 (U4): confirm before deleting; Quick Connect is not removable
            int data = ConnectSelectedItemData(hwnd);
            if (data < 0)
                return TRUE;
            if (SalamanderGeneral->SalMessageBox(hwnd, LoadStr(IDS_CONFIRM_DELETEBOOKMARK),
                                                 LoadStr(IDS_CONFIRM_DELETE_TITLE),
                                                 MB_YESNO | MB_ICONQUESTION) != IDYES)
                return TRUE;
            Config.Bookmarks.Delete(data);
            if (Config.LastBookmark == data + 1)
                Config.LastBookmark = 0;
            else if (Config.LastBookmark > data + 1)
                Config.LastBookmark--;
            ConnectFillBookmarkList(hwnd);
            ConnectSelectItemData(hwnd, QC_ITEM);
            ConnectLoadServerToFields(hwnd, &Config.QuickConnect);
            ConnectUpdateButtons(hwnd);
            return TRUE;
        }
        case IDB_CONNECT: // also "Close" in organize mode
        {
            if (d->OrganizeMode)
            {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            // feature 017 (P2/P3): connect using the current fields AND persist
            // them onto the selected entry, so a save-password-on-connect sticks.
            int data = ConnectSelectedItemData(hwnd);
            CSFTPServer* entry = ConnectEntryForItemData(data);
            if (entry == NULL)
                entry = &Config.QuickConnect; // no selection -> quick connect
            Config.LastBookmark = (data >= 0) ? data + 1 : 0;
            if (ConnectCommitToEntry(hwnd, entry, TRUE) &&
                d->Result != NULL && d->Result->CopyFrom(entry))
            {
                EndDialog(hwnd, IDOK);
            }
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL ShowConnectDialog(HWND parent, BOOL organizeMode, CSFTPServer* result)
{
    CConnectData d;
    d.OrganizeMode = organizeMode;
    d.Result = result;
    d.Hwnd = NULL;
    return DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_CONNECT), parent, ConnectProc, (LPARAM)&d) == IDOK;
}
