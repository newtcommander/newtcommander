# Quickstart: mdview HTML Rendering Surface

## Prerequisites
- Windows 11, Visual Studio 2022 (C++ Desktop workload), Windows 11 SDK.
- WebView2 Evergreen Runtime (preinstalled on Win11; no action needed).
- Vendored deps are committed under `src/common/dep/md4c/` and
  `src/common/dep/webview2/` — no download at build time.

## Build (from repo root)
```bat
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd                 :: Debug x64 incremental (default)
build.cmd rebuild         :: clean Debug x64
build.cmd full release     :: Release x64 (LTCG + codesign)
```
`plugins.cfg` already has `mdview=on`. No changes to `salamand.sln`,
`salamand.gen.slnf`, or `build.cmd`.

Build only the plugin (faster iteration), Debug x64:
```bat
msbuild src\vcxproj\salamand.sln /t:mdview /p:Configuration=Debug /p:Platform=x64 /m
```
Output: `%OPENSAL_BUILD_DIR%newtcommander\Debug_x64\plugins\mdview\mdview.spl`.
> If a running Salamander has the Debug mdview.spl loaded, relink fails
> (LNK1104) — close that instance first. The user's separate Release build does
> not lock the Debug output.

## Unit test (Markdown → HTML generator)
```bat
msbuild tests\mdview_htmlgen_test\mdview_htmlgen_test.vcxproj /p:Configuration=Debug /p:Platform=x64
:: runs md4c + htmlgen over specs/020-mdview-plugin/fixtures/*.md and
:: specs/021-mdview-html-renderer/fixtures/*.md, comparing to *.html goldens
```
This verifies the engine-independent transformation (tables, lists, code
highlight, raw-HTML pass-through, slugs, escaping, caps) without WebView2.

## Security corpus (manual/automated assertions)
Fixtures under `specs/021-mdview-html-renderer/fixtures/security/` embed
`<script>`, `onerror=`, `javascript:` links, remote `<img>`, forms, meta-refresh,
iframes, and path-traversal `src`. With the generator, assert the output never
emits a live handler in a way the locked-down engine could execute; with the
engine (runtime), assert (via `read_network_requests`-style inspection or a
WinHTTP sink) zero content-triggered network and zero script effect.

## Runtime smoke check
1. Launch the freshly built Salamander (Debug x64).
2. Plugin Manager → confirm mdview loads (`mdview.spl`).
3. F3 on a `.md` with a table, a code block, a local image, and embedded
   `<kbd>`/`<sub>` → verify: real table grid + alignment, inset body text,
   inline image, embedded HTML rendered (not literal), search (Ctrl+F), zoom
   (Ctrl+±), scheme cycle (F9).

> GUI visual confirmation is the one step that needs a human at the keyboard;
> the build + generator unit tests are fully automated.
