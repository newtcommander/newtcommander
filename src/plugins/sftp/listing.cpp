// SPDX-FileCopyrightText: 2026 Pavel Stupka
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "sftp.h"
#include "sftputils.h"
#include "session.h"
#include "listing.h"

int CSFTPListingData::RightsWidth = 0, CSFTPListingData::RightsFixed = 0;
int CSFTPListingData::OwnerWidth = 0, CSFTPListingData::OwnerFixed = 0;
int CSFTPListingData::GroupWidth = 0, CSFTPListingData::GroupFixed = 0;

// ---------------------------------------------------------------------------
// column GetText transfer variables (set once in SetupView)
// ---------------------------------------------------------------------------

static const CFileData** TransferFileData = NULL;
static int* TransferIsDir = NULL;
static char* TransferBuffer = NULL;
static int* TransferLen = NULL;
static DWORD* TransferRowData = NULL;
static CPluginDataInterfaceAbstract** TransferPluginDataIface = NULL;
static DWORD* TransferActCustomData = NULL;

static void WINAPI GetRightsText()
{
    const CSFTPItemData* d = (const CSFTPItemData*)(*TransferFileData)->PluginData;
    if (d != NULL && d->HasMode)
    {
        char buf[16];
        FormatUnixRights(d->Mode, buf);
        int n = (int)strlen(buf);
        memcpy(TransferBuffer, buf, n);
        *TransferLen = n;
    }
    else
        *TransferLen = 0;
}

static void WINAPI GetOwnerText()
{
    const CSFTPItemData* d = (const CSFTPItemData*)(*TransferFileData)->PluginData;
    if (d != NULL)
    {
        char num[32];
        const char* s = d->Owner;
        if (s == NULL || s[0] == 0)
        {
            _snprintf_s(num, _TRUNCATE, "%lu", d->Uid);
            s = num;
        }
        int n = (int)strlen(s);
        if (n > TRANSFER_BUFFER_MAX - 1)
            n = TRANSFER_BUFFER_MAX - 1;
        memcpy(TransferBuffer, s, n);
        *TransferLen = n;
    }
    else
        *TransferLen = 0;
}

static void WINAPI GetGroupText()
{
    const CSFTPItemData* d = (const CSFTPItemData*)(*TransferFileData)->PluginData;
    if (d != NULL)
    {
        char num[32];
        const char* s = d->Group;
        if (s == NULL || s[0] == 0)
        {
            _snprintf_s(num, _TRUNCATE, "%lu", d->Gid);
            s = num;
        }
        int n = (int)strlen(s);
        if (n > TRANSFER_BUFFER_MAX - 1)
            n = TRANSFER_BUFFER_MAX - 1;
        memcpy(TransferBuffer, s, n);
        *TransferLen = n;
    }
    else
        *TransferLen = 0;
}

// ---------------------------------------------------------------------------
// CSFTPListingData
// ---------------------------------------------------------------------------

CSFTPListingData::CSFTPListingData()
{
}

CSFTPListingData::~CSFTPListingData()
{
}

BOOL CSFTPListingData::FillFileData(CFileData& file, const CSFTPDirEntry* entry, BOOL* outIsDir)
{
    memset(&file, 0, sizeof(file));

    // name (UTF-8, sanitized) allocated on Salamander's heap
    char sane[3 * 256];
    SanitizeUTF8(entry->Name, sane, sizeof(sane));
    int nameLen = (int)strlen(sane);
    file.Name = (char*)SalamanderGeneral->Alloc(nameLen + 1);
    if (file.Name == NULL)
        return FALSE;
    memcpy(file.Name, sane, nameLen + 1);
    file.NameLen = nameLen;
    file.Ext = file.Name + nameLen; // set below if a dot exists

    // find extension (dot not at position 0)
    const char* dot = strrchr(file.Name, '.');
    if (dot != NULL && dot != file.Name)
        file.Ext = (char*)dot + 1;

    file.Size = CQuadWord().SetUI64(entry->HasSize ? entry->Size : 0);
    file.LastWrite.dwLowDateTime = 0;
    file.LastWrite.dwHighDateTime = 0;
    if (entry->HasMtime)
        UnixTimeToFileTime(entry->Mtime, &file.LastWrite);

    unsigned long mode = entry->Mode;
    BOOL isDir = entry->HasMode && SFTP_S_ISDIR(mode);
    BOOL isLink = entry->HasMode && SFTP_S_ISLNK(mode);

    file.Attr = SynthesizeWinAttributes(mode, sane);
    if (isDir)
        file.Attr |= FILE_ATTRIBUTE_DIRECTORY;
    file.Hidden = (sane[0] == '.') ? 1 : 0;
    file.IsLink = isLink ? 1 : 0;

    // attach per-row data
    CSFTPItemData* d = (CSFTPItemData*)malloc(sizeof(CSFTPItemData));
    if (d == NULL)
    {
        SalamanderGeneral->Free(file.Name);
        file.Name = NULL;
        return FALSE;
    }
    memset(d, 0, sizeof(*d));
    d->Mode = mode;
    d->HasMode = entry->HasMode;
    d->Uid = entry->Uid;
    d->Gid = entry->Gid;
    d->Owner = entry->Owner != NULL ? _strdup(entry->Owner) : NULL;
    d->Group = entry->Group != NULL ? _strdup(entry->Group) : NULL;
    // keep the original (unsanitized) name for all remote-path operations; the
    // panel's file.Name may have had invalid UTF-8 bytes replaced with '?'
    d->RealName = _strdup(entry->Name);
    file.PluginData = (DWORD_PTR)d;

    if (outIsDir != NULL)
        *outIsDir = isDir;
    return TRUE;
}

void CSFTPListingData::FreeItemData(CFileData& file)
{
    CSFTPItemData* d = (CSFTPItemData*)file.PluginData;
    if (d != NULL)
    {
        if (d->Owner != NULL)
            free(d->Owner);
        if (d->Group != NULL)
            free(d->Group);
        if (d->LinkTarget != NULL)
            free(d->LinkTarget);
        if (d->RealName != NULL)
            free(d->RealName);
        free(d);
        file.PluginData = 0;
    }
}

const char* SFTPRealName(const CFileData* file)
{
    if (file == NULL)
        return "";
    const CSFTPItemData* d = (const CSFTPItemData*)file->PluginData;
    if (d != NULL && d->RealName != NULL)
        return d->RealName;
    return file->Name;
}

void WINAPI CSFTPListingData::ReleasePluginData(CFileData& file, BOOL isDir)
{
    FreeItemData(file);
}

void WINAPI CSFTPListingData::SetupView(BOOL leftPanel, CSalamanderViewAbstract* view,
                                        const char* archivePath, const CFileData* upperDir)
{
    view->GetTransferVariables(TransferFileData, TransferIsDir, TransferBuffer, TransferLen,
                               TransferRowData, TransferPluginDataIface, TransferActCustomData);

    DWORD viewMode = view->GetViewMode();
    if (viewMode != VIEW_MODE_DETAILED)
        return; // custom columns only make sense in detailed mode

    // "attribute-style" view keeps the standard columns untouched (FR-021)
    if (Config.ColumnView == cvAttributes)
        return;

    // remove the standard Attributes column (index search) and add Unix columns
    int count = view->GetColumnsCount();
    for (int i = count - 1; i >= 1; i--)
    {
        const CColumn* c = view->GetColumn(i);
        if (c != NULL && c->ID == COLUMN_ID_ATTRIBUTES)
            view->DeleteColumn(i);
    }

    // insert Rights, Owner, Group after the standard columns
    int insertAt = view->GetColumnsCount();

    CColumn col;

    memset(&col, 0, sizeof(col));
    lstrcpynA(col.Name, LoadStr(IDS_COL_RIGHTS), COLUMN_NAME_MAX);
    lstrcpynA(col.Description, LoadStr(IDS_COL_RIGHTS_DESC), COLUMN_DESCRIPTION_MAX);
    col.GetText = GetRightsText;
    col.CustomData = SFTPCOL_RIGHTS;
    col.SupportSorting = 1;
    col.LeftAlignment = 1;
    col.ID = COLUMN_ID_CUSTOM;
    col.Width = RightsWidth ? RightsWidth : 90;
    col.FixedWidth = RightsFixed;
    view->InsertColumn(insertAt++, &col);

    memset(&col, 0, sizeof(col));
    lstrcpynA(col.Name, LoadStr(IDS_COL_OWNER), COLUMN_NAME_MAX);
    lstrcpynA(col.Description, LoadStr(IDS_COL_OWNER_DESC), COLUMN_DESCRIPTION_MAX);
    col.GetText = GetOwnerText;
    col.CustomData = SFTPCOL_OWNER;
    col.SupportSorting = 1;
    col.LeftAlignment = 1;
    col.ID = COLUMN_ID_CUSTOM;
    col.Width = OwnerWidth ? OwnerWidth : 80;
    col.FixedWidth = OwnerFixed;
    view->InsertColumn(insertAt++, &col);

    memset(&col, 0, sizeof(col));
    lstrcpynA(col.Name, LoadStr(IDS_COL_GROUP), COLUMN_NAME_MAX);
    lstrcpynA(col.Description, LoadStr(IDS_COL_GROUP_DESC), COLUMN_DESCRIPTION_MAX);
    col.GetText = GetGroupText;
    col.CustomData = SFTPCOL_GROUP;
    col.SupportSorting = 1;
    col.LeftAlignment = 1;
    col.ID = COLUMN_ID_CUSTOM;
    col.Width = GroupWidth ? GroupWidth : 80;
    col.FixedWidth = GroupFixed;
    view->InsertColumn(insertAt++, &col);
}

void WINAPI CSFTPListingData::ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn* column, int newFixedWidth)
{
    switch (column->CustomData)
    {
    case SFTPCOL_RIGHTS: RightsFixed = newFixedWidth; if (newFixedWidth) RightsWidth = column->Width; break;
    case SFTPCOL_OWNER: OwnerFixed = newFixedWidth; if (newFixedWidth) OwnerWidth = column->Width; break;
    case SFTPCOL_GROUP: GroupFixed = newFixedWidth; if (newFixedWidth) GroupWidth = column->Width; break;
    }
}

void WINAPI CSFTPListingData::ColumnWidthWasChanged(BOOL leftPanel, const CColumn* column, int newWidth)
{
    switch (column->CustomData)
    {
    case SFTPCOL_RIGHTS: RightsWidth = newWidth; break;
    case SFTPCOL_OWNER: OwnerWidth = newWidth; break;
    case SFTPCOL_GROUP: GroupWidth = newWidth; break;
    }
}

BOOL WINAPI CSFTPListingData::GetInfoLineContent(int panel, const CFileData* file, BOOL isDir, int selectedFiles,
                                                 int selectedDirs, BOOL displaySize, const CQuadWord& selectedSize,
                                                 char* buffer, DWORD* hotTexts, int& hotTextsCount)
{
    if (file == NULL)
        return FALSE; // standard multi-selection info line
    const CSFTPItemData* d = (const CSFTPItemData*)file->PluginData;
    if (d == NULL || !d->HasMode)
        return FALSE;

    char rights[16];
    FormatUnixRights(d->Mode, rights);

    char octal[8];
    char line[1000];
    if (Config.ShowOctal)
    {
        FormatOctalMode(d->Mode, octal);
        _snprintf_s(line, _TRUNCATE, "%s (%s)  %s:%s",
                    rights, octal,
                    d->Owner != NULL ? d->Owner : "?",
                    d->Group != NULL ? d->Group : "?");
    }
    else
    {
        _snprintf_s(line, _TRUNCATE, "%s  %s:%s", rights,
                    d->Owner != NULL ? d->Owner : "?",
                    d->Group != NULL ? d->Group : "?");
    }
    if (d->LinkTarget != NULL)
    {
        size_t l = strlen(line);
        _snprintf_s(line + l, sizeof(line) - l, _TRUNCATE, " -> %s", d->LinkTarget);
    }
    lstrcpynA(buffer, line, 1000);
    hotTextsCount = 0;
    return TRUE;
}
