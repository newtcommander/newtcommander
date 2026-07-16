// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ****************************************************************************
// Global plugin state, configuration, and the saved-connection model.
// ****************************************************************************

// language-independent (.spl) and language (.slg) module handles
extern HINSTANCE DLLInstance;
extern HINSTANCE HLanguage;

// generic Salamander services, valid for the whole plugin lifetime
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;

// FS name assigned to us by Salamander (usually "sftp")
extern char AssignedFSName[MAX_PATH];
extern int AssignedFSNameLen;

// small icons loaded in CPluginInterface::Connect
extern HICON SFTPIcon;
extern HICON SFTPLogIcon;

// loads a localized string from the .slg module
char* LoadStr(int resID);

// frequently used
extern const char* LOW_MEMORY;

// default TCP port for SSH
#define SFTP_DEFAULT_PORT 22

// authentication method stored in a connection profile
enum CSFTPAuthMethod
{
    saPassword = 0,
    saPrivateKey = 1,
};

// permission-column display mode (FR-021)
enum CSFTPColumnView
{
    cvUnixRights = 0,
    cvAttributes = 1,
};

// ****************************************************************************
// CSFTPServer - one saved connection (bookmark) or the quick-connect entry.
// Secrets are held as password-manager blobs, never as plaintext at rest.
// ****************************************************************************

class CSFTPServer
{
public:
    char* ItemName;    // bookmark display name (NULL for quick-connect)
    char* Address;     // host name or IP
    int Port;          // TCP port (default 22)
    char* UserName;    // login user (may be NULL/empty -> prompt)
    int AuthMethod;    // CSFTPAuthMethod

    BYTE* EncryptedPassword; // password-manager blob or NULL
    int EncryptedPasswordSize;
    BOOL SavePassword;

    char* KeyFile;              // private-key path (saPrivateKey)
    BYTE* EncryptedPassphrase;  // password-manager blob or NULL
    int EncryptedPassphraseSize;
    BOOL SavePassphrase;

    char* InitialPath;     // initial remote path (NULL/empty -> home)
    char* TargetPanelPath; // optional local path for the other panel

    int KeepAliveSendEvery; // seconds, 0 = use global
    int KeepAliveStopAfter;  // minutes, 0 = use global
    BOOL UseCompression;

public:
    CSFTPServer();
    ~CSFTPServer();

    void Clear();
    BOOL CopyFrom(const CSFTPServer* src); // deep copy; FALSE on low memory
    BOOL Set(const char* itemName, const char* address, int port, const char* user);

    // helpers to (re)assign heap strings; return FALSE on low memory
    BOOL SetString(char** target, const char* value);
    BOOL SetBlob(BYTE** target, int* targetSize, const BYTE* data, int size);
};

class CSFTPServerList : public TIndirectArray<CSFTPServer>
{
public:
    CSFTPServerList() : TIndirectArray<CSFTPServer>(10, 10) {}

    // re-encrypt all stored password/passphrase blobs after a master-password
    // change (PasswordManagerEvent)
    void EncryptPasswords(HWND parent, BOOL encrypt);
};

// ****************************************************************************
// CSFTPConfig - global plugin configuration (persisted under the plugin key).
// ****************************************************************************

class CSFTPConfig
{
public:
    int Version;

    int ConnectTimeout;    // seconds
    int OperationTimeout;  // seconds
    int KeepAliveSendEvery; // seconds
    int KeepAliveStopAfter; // minutes
    int ConnectRetries;
    int RetryDelay; // seconds

    int ColumnView; // CSFTPColumnView
    BOOL ShowOctal;

    int ResumeOverlap;    // bytes re-read on resume
    int ResumeMinFileSize; // below this, restart instead of resume

    BOOL EnableLogging;
    int LogMaxSize; // KB

    int LastBookmark; // 0 = quick connect

    CSFTPServerList Bookmarks;
    CSFTPServer QuickConnect;

    // dialog placements
    WINDOWPLACEMENT ConnectDlgPlacement;
    WINDOWPLACEMENT LogsDlgPlacement;

public:
    CSFTPConfig();
    void SetDefaults();
};

extern CSFTPConfig Config;

// ****************************************************************************
// The four plugin interfaces (defined in sftp.cpp / fs.cpp).
// ****************************************************************************

class CPluginInterfaceForFS;
class CPluginInterfaceForMenuExt;

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent);
    virtual BOOL WINAPI Release(HWND parent, BOOL force);
    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI Configuration(HWND parent);
    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);
    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData);

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() { return NULL; }
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt();
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS();
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent) {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}
    virtual void WINAPI PasswordManagerEvent(HWND parent, int event);
};

// menu command IDs
#define SFTPCMD_CONNECT 1
#define SFTPCMD_ORGANIZEBOOKMARKS 2
#define SFTPCMD_SHOWLOGS 3
#define SFTPCMD_CREATESYMLINK 4

class CPluginInterfaceForMenuExt : public CPluginInterfaceForMenuExtAbstract
{
public:
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask);
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                        int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id) { return FALSE; }
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander) {}
};

extern CPluginInterface PluginInterface;
