// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

struct CSFTPDirEntry;

// per-row data hung off CFileData::PluginData (freed in ReleasePluginData)
struct CSFTPItemData
{
    unsigned long Mode; // permissions incl. type + special bits (0 = unknown)
    unsigned long Uid;
    unsigned long Gid;
    char* Owner;        // heap owner name or NULL (numeric fallback used)
    char* Group;        // heap group name or NULL
    char* LinkTarget;   // heap symlink target or NULL
    BOOL LinkIsDir;
    BOOL LinkBroken;
    BOOL HasMode;
};

// custom column discriminators (stored in CColumn::CustomData)
#define SFTPCOL_RIGHTS 1
#define SFTPCOL_OWNER 2
#define SFTPCOL_GROUP 3

// ****************************************************************************
// CSFTPListingData - supplies the Rights/Owner/Group custom columns and owns
// the per-row CSFTPItemData blocks for one directory listing.
// ****************************************************************************

class CSFTPListingData : public CPluginDataInterfaceAbstract
{
protected:
    // saved fixed-width/width for the custom columns (persisted via Config-less
    // in-memory statics; simple and matches FTP's per-session behavior)
    static int RightsWidth, RightsFixed;
    static int OwnerWidth, OwnerFixed;
    static int GroupWidth, GroupFixed;

public:
    CSFTPListingData();
    virtual ~CSFTPListingData();

    // allocates a CSFTPItemData from a listing entry and attaches it to 'file'
    static BOOL FillFileData(CFileData& file, const CSFTPDirEntry* entry, BOOL* outIsDir);
    static void FreeItemData(CFileData& file);

    virtual BOOL WINAPI CallReleaseForFiles() { return TRUE; }
    virtual BOOL WINAPI CallReleaseForDirs() { return TRUE; }
    virtual void WINAPI ReleasePluginData(CFileData& file, BOOL isDir);
    virtual void WINAPI GetFileDataForUpDir(const char* archivePath, CFileData& upDir) {}
    virtual BOOL WINAPI GetFileDataForNewDir(const char* dirName, CFileData& dir) { return FALSE; }
    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize) { return NULL; }
    virtual BOOL WINAPI HasSimplePluginIcon(CFileData& file, BOOL isDir) { return TRUE; }
    virtual HICON WINAPI GetPluginIcon(const CFileData* file, int iconSize, BOOL& destroyIcon) { return NULL; }
    virtual int WINAPI CompareFilesFromFS(const CFileData* file1, const CFileData* file2)
    {
        return strcmp(file1->Name, file2->Name);
    }
    virtual void WINAPI SetupView(BOOL leftPanel, CSalamanderViewAbstract* view,
                                  const char* archivePath, const CFileData* upperDir);
    virtual void WINAPI ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn* column, int newFixedWidth);
    virtual void WINAPI ColumnWidthWasChanged(BOOL leftPanel, const CColumn* column, int newWidth);
    virtual BOOL WINAPI GetInfoLineContent(int panel, const CFileData* file, BOOL isDir, int selectedFiles,
                                           int selectedDirs, BOOL displaySize, const CQuadWord& selectedSize,
                                           char* buffer, DWORD* hotTexts, int& hotTextsCount);
    virtual BOOL WINAPI CanBeCopiedToClipboard() { return TRUE; }
    virtual BOOL WINAPI GetByteSize(const CFileData* file, BOOL isDir, CQuadWord* size) { return FALSE; }
    virtual BOOL WINAPI GetLastWriteDate(const CFileData* file, BOOL isDir, SYSTEMTIME* date) { return FALSE; }
    virtual BOOL WINAPI GetLastWriteTime(const CFileData* file, BOOL isDir, SYSTEMTIME* time) { return FALSE; }
};
