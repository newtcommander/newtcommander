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
        SetDlgItemTextA(hwnd, IDT_PROMPTTEXT, d->Prompt);
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
        SetDlgItemTextA(hwnd, IDT_HOSTKEY_TEXT, text);
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
        SetDlgItemTextA(hwnd, IDT_RENAMEPROMPT, d->Prompt);
        SetDlgItemTextA(hwnd, IDE_RENAMENAME, d->Name);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        SetFocus(GetDlgItem(hwnd, IDE_RENAMENAME));
        return FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemTextA(hwnd, IDE_RENAMENAME, d->Name, MAX_PATH);
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
        SetDlgItemTextA(hwnd, IDE_SYMLINKNAME, d->Name);
        SetDlgItemTextA(hwnd, IDE_SYMLINKTARGET, d->Target);
        SalamanderGeneral->MultiMonCenterWindow(hwnd, GetParent(hwnd), TRUE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemTextA(hwnd, IDE_SYMLINKNAME, d->Name, MAX_PATH);
            GetDlgItemTextA(hwnd, IDE_SYMLINKTARGET, d->Target, MAX_PATH);
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
    if (IsDlgButtonChecked(hwnd, IDC_UR)) m |= 0400;
    if (IsDlgButtonChecked(hwnd, IDC_UW)) m |= 0200;
    if (IsDlgButtonChecked(hwnd, IDC_UX)) m |= 0100;
    if (IsDlgButtonChecked(hwnd, IDC_GR)) m |= 0040;
    if (IsDlgButtonChecked(hwnd, IDC_GW)) m |= 0020;
    if (IsDlgButtonChecked(hwnd, IDC_GX)) m |= 0010;
    if (IsDlgButtonChecked(hwnd, IDC_OR)) m |= 0004;
    if (IsDlgButtonChecked(hwnd, IDC_OW)) m |= 0002;
    if (IsDlgButtonChecked(hwnd, IDC_OX)) m |= 0001;
    if (IsDlgButtonChecked(hwnd, IDC_SETUID)) m |= 04000;
    if (IsDlgButtonChecked(hwnd, IDC_SETGID)) m |= 02000;
    if (IsDlgButtonChecked(hwnd, IDC_STICKY)) m |= 01000;
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
        SetDlgItemTextA(hwnd, IDT_CHMODTARGET, d->Label);
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
        case IDC_UR: case IDC_UW: case IDC_UX:
        case IDC_GR: case IDC_GW: case IDC_GX:
        case IDC_OR: case IDC_OW: case IDC_OX:
        case IDC_SETUID: case IDC_SETGID: case IDC_STICKY:
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
    SetDlgItemTextA(hwnd, IDE_HOSTADDRESS, s->Address != NULL ? s->Address : "");
    SetDlgItemInt(hwnd, IDE_PORT, s->Port > 0 ? s->Port : SFTP_DEFAULT_PORT, FALSE);
    SetDlgItemTextA(hwnd, IDE_USERNAME, s->UserName != NULL ? s->UserName : "");
    CheckRadioButton(hwnd, IDC_AUTHPASSWORD, IDC_AUTHKEY,
                     s->AuthMethod == saPrivateKey ? IDC_AUTHKEY : IDC_AUTHPASSWORD);
    SetDlgItemTextA(hwnd, IDE_PASSWORD, "");
    CheckDlgButton(hwnd, IDC_SAVEPASSWORD, s->SavePassword ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextA(hwnd, IDE_KEYFILE, s->KeyFile != NULL ? s->KeyFile : "");
    SetDlgItemTextA(hwnd, IDE_PASSPHRASE, "");
    CheckDlgButton(hwnd, IDC_SAVEPASSPHRASE, s->SavePassphrase ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextA(hwnd, IDE_INITIALPATH, s->InitialPath != NULL ? s->InitialPath : "");
    ConnectSetAuthMode(hwnd, s->AuthMethod);
}

// reads dialog fields into 's'; encrypts typed secrets and also copies the
// plaintext into ConnectPlainPassword/Passphrase for the immediate connect
static BOOL ConnectReadFields(HWND hwnd, CSFTPServer* s, const CSFTPServer* selectedBookmark)
{
    char host[256], user[256], keyfile[MAX_PATH], initpath[1024];
    GetDlgItemTextA(hwnd, IDE_HOSTADDRESS, host, sizeof(host));
    GetDlgItemTextA(hwnd, IDE_USERNAME, user, sizeof(user));
    GetDlgItemTextA(hwnd, IDE_KEYFILE, keyfile, sizeof(keyfile));
    GetDlgItemTextA(hwnd, IDE_INITIALPATH, initpath, sizeof(initpath));
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

    ConnectPlainPassword[0] = 0;
    ConnectPlainPassphrase[0] = 0;

    CSalamanderPasswordManagerAbstract* pm = SalamanderGeneral->GetSalamanderPasswordManager();

    if (authMethod == saPassword)
    {
        char pwd[512];
        GetDlgItemTextA(hwnd, IDE_PASSWORD, pwd, sizeof(pwd));
        if (pwd[0] != 0)
        {
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
        else if (selectedBookmark != NULL && selectedBookmark->SavePassword &&
                 selectedBookmark->EncryptedPassword != NULL)
        {
            // reuse the stored blob
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
        else if (selectedBookmark != NULL && selectedBookmark->SavePassphrase &&
                 selectedBookmark->EncryptedPassphrase != NULL)
        {
            s->SetBlob(&s->EncryptedPassphrase, &s->EncryptedPassphraseSize,
                       selectedBookmark->EncryptedPassphrase, selectedBookmark->EncryptedPassphraseSize);
        }
    }
    return TRUE;
}

static void ConnectFillBookmarkList(HWND hwnd)
{
    HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
    SendMessage(lb, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < Config.Bookmarks.Count; i++)
    {
        CSFTPServer* s = Config.Bookmarks[i];
        const char* name = (s->ItemName != NULL && s->ItemName[0]) ? s->ItemName :
                           (s->Address != NULL ? s->Address : "(unnamed)");
        int idx = (int)SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)name);
        SendMessage(lb, LB_SETITEMDATA, idx, i);
    }
}

static void BrowseForKey(HWND hwnd)
{
    char file[MAX_PATH];
    GetDlgItemTextA(hwnd, IDE_KEYFILE, file, sizeof(file));
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "All files\0*.*\0Private keys\0*.pem;*.ppk;*.key;id_*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameA(&ofn))
        SetDlgItemTextA(hwnd, IDE_KEYFILE, file);
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
        // seed fields from quick-connect or the last bookmark
        if (Config.LastBookmark > 0 && Config.LastBookmark <= Config.Bookmarks.Count)
        {
            int bi = Config.LastBookmark - 1;
            ConnectLoadServerToFields(hwnd, Config.Bookmarks[bi]);
            SendMessage(GetDlgItem(hwnd, IDL_BOOKMARKS), LB_SETCURSEL, bi, 0);
        }
        else
            ConnectLoadServerToFields(hwnd, &Config.QuickConnect);
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
                int sel = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR)
                {
                    int bi = (int)SendMessage((HWND)lParam, LB_GETITEMDATA, sel, 0);
                    if (bi >= 0 && bi < Config.Bookmarks.Count)
                        ConnectLoadServerToFields(hwnd, Config.Bookmarks[bi]);
                }
            }
            return TRUE;
        case IDB_NEWBOOKMARK:
        {
            char name[MAX_PATH] = "";
            if (ShowRenameDialog(hwnd, "Bookmark name:", name) && name[0] != 0)
            {
                CSFTPServer* s = new CSFTPServer;
                if (s != NULL && ConnectReadFields(hwnd, s, NULL))
                {
                    s->SetString(&s->ItemName, name);
                    Config.Bookmarks.Add(s);
                    ConnectFillBookmarkList(hwnd);
                    SendMessage(GetDlgItem(hwnd, IDL_BOOKMARKS), LB_SETCURSEL, Config.Bookmarks.Count - 1, 0);
                }
                else
                    delete s;
            }
            return TRUE;
        }
        case IDB_RENAMEBOOKMARK:
        {
            HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
            int sel = (int)SendMessage(lb, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR)
            {
                int bi = (int)SendMessage(lb, LB_GETITEMDATA, sel, 0);
                CSFTPServer* s = Config.Bookmarks[bi];
                char name[MAX_PATH];
                lstrcpynA(name, s->ItemName != NULL ? s->ItemName : "", sizeof(name));
                if (ShowRenameDialog(hwnd, "Bookmark name:", name) && name[0] != 0)
                {
                    s->SetString(&s->ItemName, name);
                    ConnectFillBookmarkList(hwnd);
                    SendMessage(lb, LB_SETCURSEL, sel, 0);
                }
            }
            return TRUE;
        }
        case IDB_REMOVEBOOKMARK:
        {
            HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
            int sel = (int)SendMessage(lb, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR)
            {
                int bi = (int)SendMessage(lb, LB_GETITEMDATA, sel, 0);
                Config.Bookmarks.Delete(bi);
                ConnectFillBookmarkList(hwnd);
            }
            return TRUE;
        }
        case IDB_CONNECT: // also "Close" in organize mode
        {
            if (d->OrganizeMode)
            {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            HWND lb = GetDlgItem(hwnd, IDL_BOOKMARKS);
            int sel = (int)SendMessage(lb, LB_GETCURSEL, 0, 0);
            const CSFTPServer* bookmark = NULL;
            if (sel != LB_ERR)
            {
                int bi = (int)SendMessage(lb, LB_GETITEMDATA, sel, 0);
                if (bi >= 0 && bi < Config.Bookmarks.Count)
                {
                    bookmark = Config.Bookmarks[bi];
                    Config.LastBookmark = bi + 1;
                }
            }
            else
                Config.LastBookmark = 0;
            if (d->Result != NULL && ConnectReadFields(hwnd, d->Result, bookmark))
            {
                // remember quick-connect field values
                if (bookmark == NULL)
                    Config.QuickConnect.CopyFrom(d->Result);
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
