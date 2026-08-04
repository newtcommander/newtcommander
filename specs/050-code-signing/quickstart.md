# Quickstart: Signed Releases

## Prerequisites (one-time)

- Certum code-signing certificate installed in your Windows certificate
  store (current user, `My`), with its SHA-1 thumbprint recorded in
  `tools\codesign\codesign.cfg`.
- Windows 10/11 SDK (signtool) — already a build prerequisite.
- Inno Setup 7 (`C:\Program Files\Inno Setup 7\ISCC.exe`) for the installer.

## Everyday development (nothing changes)

```batch
build.cmd                 :: Debug — never signed
build.cmd full release    :: Release — unsigned, fast, no TSA contact
```

## Produce a fully signed release + installer (one command)

```batch
build.cmd full release sign setup
```

Build → sign all 206 PE artifacts (timestamped) → verify → compile signed
installer (installer + uninstaller signed) into
`setup\output\tandemcommander-0.1.0-x64-setup.exe`.

## Individual steps

```batch
build.cmd full release sign               :: signed build only
setup\build_setup.cmd                     :: unsigned installer (dev test)
setup\build_setup.cmd sign                :: sign tree if needed + signed installer

:: sign an existing (already built) tree without rebuilding:
powershell -NoProfile -ExecutionPolicy Bypass -File tools\codesign\sign_release.ps1 ^
    -Root "%OPENSAL_BUILD_DIR%tandemcommander\Release_x64"

:: check signing state without modifying anything:
powershell -NoProfile -ExecutionPolicy Bypass -File tools\codesign\sign_release.ps1 ^
    -Root "%OPENSAL_BUILD_DIR%tandemcommander\Release_x64" -VerifyOnly
```

(If `OPENSAL_BUILD_DIR` is unset, the tree is `<repo>\build\tandemcommander\Release_x64`.)

## Certificate rotation

1. Install the new certificate into the Windows certificate store.
2. Edit `tools\codesign\codesign.cfg` → set `thumbprint` to the new value.
3. Re-run signing (any of the commands above) — every artifact still signed
   with the old certificate is re-signed automatically.

## Verifying by hand

```powershell
Get-AuthenticodeSignature "$env:OPENSAL_BUILD_DIR\tandemcommander\Release_x64\tandemcommander.exe"
signtool verify /pa "setup\output\tandemcommander-0.1.0-x64-setup.exe"
```

## Troubleshooting

- **"certificate not found"** — the thumbprint in `codesign.cfg` is not in
  your user/machine `My` store; `Get-ChildItem Cert:\CurrentUser\My` to list.
- **timestamp failures** — Certum hiccup; the sweep retries 3× per batch and
  falls back per-file. Re-running is safe and only signs what's missing.
- **`sign` without `release`** — rejected by design; Debug builds are never
  signed.
