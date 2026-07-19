# mdview: přepis vykreslování na HTML view — technická analýza

**Datum**: 2026-07-19 · **Stav**: analýza (žádné změny kódu)
**Vztah k feature 020**: post-v1 studie; navazuje na `spec.md` (Decisions Log),
`analysis/integration.md`, `analysis/security.md`, `analysis/visual.md`
**Výchozí stav kódu**: commit `15c94b3` ([020] Add mdview plugin)

---

## 1. Účel a vazba na feature 020

Plugin mdview v1 vykresluje Markdown vlastním parserem přes RTF do RichEdit
kontrolu. Funguje, ale narazil na strop: text není odsazen od krajů okna,
tabulky se kreslí jen jako monospaced ASCII, obrázky jsou placeholder,
vložené raw HTML se zobrazuje jako doslovný text. Tato analýza zkoumá
**přepis vykreslovací vrstvy na HTML view komponentu**, který by přinesl:

- plnohodnotné vykreslení tabulek (mřížka, zarovnání sloupců, barvy),
- typografii a okraje (reading measure, odsazení, kvalitní fonty),
- nativní zobrazení vloženého raw HTML,
- inline obrázky,
- budoucí rozšiřitelnost (mermaid diagramy, matematika, export do PDF,
  „obohacení" pohledu — TOC, kopírování kódu tlačítkem apod.).

Okrajová podmínka projektu (konstituce I, `CLAUDE.md`): **build zůstává
samonosný** — všechny doplňkové knihovny budou součástí zdrojáků
(`src/common/dep/`), žádné NuGet/vcpkg, `build.cmd` musí projít na čistém
stroji jen s VS2022.

### 1.1 Governance gate — co je nutné formálně změnit

Spec 020 obsahuje dvě klarifikační rozhodnutí (Decisions Log, session
2026-07-18), která tento přepis přímo blokují a vyžadují **formální
amendment** (návrh textu je v sekci 9):

| Rozhodnutí | Současné znění | Dopad na přepis |
|---|---|---|
| **Q1** (FR-015) | Raw HTML = inert literal text | Nativní zobrazení HTML vyžaduje amendment Q1 — u variant A i B |
| **Q2** (FR-046) | Statický script-free renderer; „A browser engine is NOT used in v1" | WebView2 (varianta A) vyžaduje amendment Q2; litehtml (varianta B) NE — je to stále statický script-free renderer |

`analysis/security.md:98-107` navíc předepisuje: pokud je zvolen browser
engine, plán MUSÍ obsahovat **per-invariant enforcement tabulku +
re-verifikační strategii** napříč auto-updaty enginu. Tato tabulka je
v sekci 4.1.5.

---

## 2. Audit současného stavu

### 2.1 Pipeline v1

```
soubor (bytes)
  → MdDetectDecode (render.cpp:137)        BOM UTF-8/16LE/BE, striktní UTF-8
                                           validace, CP_ACP fallback
  → MdRenderMarkdown (render.cpp:642)      ruční line-based parser
                                           (podmnožina CommonMark 0.31.2
                                           + 4 GFM rozšíření)
  → RTF string (BuildHeader render.cpp:224, dvoufontová tabulka
                Segoe UI / Consolas, 23 barev z aktivního schématu)
  → RICHEDIT50W child (Msftedit.dll)       EM_STREAMIN (viewer.cpp:369)
  → post-pass: CFE_LINK přes zaznamenané CHARRANGE (viewer.cpp:437-455)
```

Rozsah: ~1 941 řádků .cpp (`render.cpp` 807, `viewer.cpp` 749, `mdview.cpp`
207, `highlight.cpp` 178) + hlavičky a resources. Žádná third-party
knihovna — parser, RTF emise i highlighter jsou ručně psané.

### 2.2 Co dnes RichEdit dává „zdarma"

- scroll (`WS_VSCROLL`, `ES_AUTOVSCROLL`), reflow při resize,
- selekce textu + Ctrl+C (plain text),
- hledání `EM_FINDTEXTEXW` s wrap-around (`DoFind`, viewer.cpp:500),
- zoom `EM_SETZOOM` 50–300 % (viewer.cpp:465), Ctrl+kolečko.

Cokoli RichEdit nahradí, musí tyto čtyři věci zachovat (feature-parity,
sekce 3).

### 2.3 Slabiny v1 (přesně to, co uživatel reklamuje)

| Slabina | Kde v kódu | Důsledek |
|---|---|---|
| Tabulky jako ASCII text | `emitTable` (render.cpp:585) — sloupce paddované mezerami v Consolas | Žádná mřížka, žádné zarovnání (`:` markery se detekují, ale neaplikují), rozpadá se při zalomení |
| Žádné okraje / reading measure | RTF bez `\margl`, RichEdit bez `EM_SETRECT` | Text nalepený na kraj okna; D9 (~900 px measure) nerealizováno |
| Obrázky jen placeholder | render.cpp:351 — `□ alt/src` | FR obrázků odloženo |
| Raw HTML = literal | fallback do odstavce (render.cpp:779) | `<b>`, `<br>`, bloky HTML se zobrazí jako zdroják |
| Typografie | dvoufontový RTF | Bez ligatur/kerningu/emoji fallbacku, hrubé nadpisy |

### 2.4 Opravy dříve tradovaných faktů (ověřeno v repu)

1. `highlight.cpp` **neemituje HTML** — emituje `HlRun {start, len,
   colorCf}` runy vázané na RTF color indexy `MDCF_*` (render.h:100-118).
   HTML vrstva potřebuje malý adaptér `MDCF_*` → CSS třídy (`hl-kw`,
   `hl-str`, …).
2. Adresář `src\plugins\shared\libs\` **neexistuje** — je to jen rezervovaná
   linker path v `plugin_base.props:30`. Žádný plugin dnes nelinkuje
   prebuilt `.lib`; konvenci by teprve zaváděl WebView2 loader (řešeno
   dep-lokálně, sekce 7).
3. mdview se staví **Win32 i x64** (4 konfigurace v `mdview.vcxproj`)
   → každá binární závislost musí existovat pro obě architektury.
4. Pluginy linkují **/MD, resp. /MDd** (`plugin_release.props:14`) — CRT
   kompatibilita `WebView2LoaderStatic.lib` je nutný spike (OQ-7).
5. Současný allowlist odkazů obsahuje **`ftp`** (viewer.cpp:589) — deviace
   od invariantu 6 (povoluje jen http/https/mailto). Přepis je příležitost
   ftp odstranit (OQ-6).

---

## 3. Přenášené požadavky

### 3.1 Feature-parity checklist (musí přežít výměnu enginu)

| # | Featura | Dnešní mechanismus |
|---|---|---|
| FP-1 | Hledání Ctrl+F, F3/Shift+F3, wrap-around | `EM_FINDTEXTEXW` + `IDD_FIND` |
| FP-2 | Zoom 50–300 %, Ctrl+±/0, Ctrl+kolečko, perzistence | `EM_SETZOOM`, `g_zoom` v registry |
| FP-3 | 10 barevných schémat (5 světlých/5 tmavých, WCAG), F9 cyklus, follow-system | `MdThemes[]` (render.cpp:21), `EffectiveTheme` |
| FP-4 | Interní kotvy `#slug` (GitHub slugger, česká diakritika zachována) | slug tabulka + `EM_SCROLLCARET` (viewer.cpp:559) |
| FP-5 | Lokální `.md`/`.markdown` odkazy → **nové okno vieweru** (Q3) | `SpawnViewer` (viewer.cpp:605) |
| FP-6 | Externí odkazy: gesto + allowlist schémat | `ActivateLinkByCp` gate (viewer.cpp:589) |
| FP-7 | Ostatní lokální soubory: jen zobrazit cestu (nespouštět) | `IDS_LINK_BLOCKED` větev |
| FP-8 | Encoding: BOM UTF-8/16LE/BE, UTF-8 validace, ANSI fallback | `MdDetectDecode` — **zůstává beze změny** |
| FP-9 | Long paths `\\?\`, žádné MAX_PATH buffery | `SplU8ToWExtAlloc` |
| FP-10 | Size gate 20 MB → nabídka OpenAsText | `SIZE_GATE` (viewer.cpp:18) |
| FP-11 | Fallback Ctrl+U / chyby → interní textový viewer | `OpenAsText` → `ViewFileInPluginViewer` |
| FP-12 | Okno-per-dokument, vlastní thread + message loop, WINDOWPLACEMENT | `CViewerThread` (viewer.cpp:92) — **zůstává beze změny** |
| FP-13 | Selekce textu + Ctrl+C (plain text) | nativní RichEdit |

### 3.2 Bezpečnostní invarianty (security.md:109-132, nepodkročitelné)

1. Rendering nikdy nespustí žádný skript.
2. Rendering nikdy nespustí příkaz/aplikaci/protocol handler.
3. Rendering neprovádí žádné síťové I/O bez následného explicitního gesta.
4. Žádný HTML event handler se nikdy nenaváže; iframe/object/embed/forms/
   meta-refresh nikdy aktivní.
5. Lokální zdroje z obsahu: read-only, jen relativní cesty kanonizované
   proti adresáři dokumentu; absolute/UNC/device/`file:` odmítnuto.
6. Aktivace odkazu jen gestem; schémata allowlist (http/https/mailto).
7. Vstup libovolné velikosti/zanoření/malformace nesmí shodit UI thread,
   zamrznout ani vyčerpat paměť; degradace do chyby/text-vieweru.
8. Clipboard jen na explicitní příkaz, jen plain text.
9. SVG (pokud) přes statický, script-free, non-fetching rasterizer.
10. Všechny politiky se znovu aplikují při každém reloadu.

---

## 4. Varianty

### 4.0 Srovnávací matice

| Kritérium | A: WebView2 | B: litehtml | C: RichEdit evoluce | D: MSHTML |
|---|---|---|---|---|
| Tabulky (mřížka, zarovnání) | ✔ plná CSS | ✔ dobrá (CSS2) | ~ RTF `\trowd` | ✔ (IE11) |
| Okraje / reading measure | ✔ CSS | ✔ CSS | ~ `EM_SETRECT` | ✔ |
| Raw HTML nativně | ✔ plně | ✔ převážně | ✘ nikdy | ✔ |
| Inline obrázky | ✔ engine | ✔ WIC+nanosvg | ~ `EM_INSERTIMAGE` | ✔ |
| Selekce + Ctrl+C | ✔ nativní | ✘ dopsat (~800–1500 LOC) | ✔ nativní | ✔ |
| Hledání | ✔ Find API (+fallback) | ✘ dopsat (~300–500 LOC) | ✔ nativní | ~ |
| Zoom | ✔ ZoomFactor | ~ re-render | ✔ nativní | ~ |
| Mermaid/math/PDF export v budoucnu | ✔ reálné | ✘ nereálné | ✘ | ✘ |
| Invarianty 1/2/4 | konfigurací + testy | **by construction** | by construction | ✘ neudržitelné |
| Invariant 7 (izolace parsingu) | ✔ out-of-process sandbox | ✘ in-process, plná práva | ~ in-process | ✘ |
| Build samonosný | SDK v repu; **engine = OS komponenta** | ✔ 100 % v repu | ✔ (OS kontrola) | ✔ (OS, mrtvé) |
| Procesy navíc / RAM | msedgewebview2.exe strom, ~100–200 MB | 0 | 0 | 0 |
| Nový kód (odhad) | ~1 500–2 200 LOC | ~2 800–4 400 LOC | ~800–1 200 LOC | — |
| Vendorovaný kód | ~3 kLOC (md4c) + hlavičky + 2×.lib | ~3 kLOC (md4c) + desítky kLOC (litehtml+gumbo) | 0 | 0 |
| Licence | SDK BSD-3 ✔; runtime = OS | litehtml BSD-3 ✔; gumbo **Apache-2.0** (OQ-3) | — | — |
| Amendment specu | Q1 + Q2 | jen Q1 | žádný | — |
| Verdikt | **doporučeno** | contingency | baseline/mezikrok | reject |

### 4.1 Varianta A — md4c → HTML → WebView2 (doporučená)

#### 4.1.1 Pipeline

```
bytes → MdDetectDecode (beze změny) → UTF-16 → UTF-8
  → md4c (MD_DIALECT_GITHUB; NOHTML vypnuto = raw HTML pass-through)
  → htmlgen.cpp (vlastní MD_PARSER renderer, sekce 5)
  → HTML dokument + vygenerovaný CSS (téma, zoom-nezávislý)
  → ICoreWebView2.Navigate("https://mdview.invalid/doc.html")
       obslouženo interceptorem WebResourceRequested z paměti (IStream)
```

**Proč ne jednodušší cesty servírování:**

- `NavigateToString` má ~2MB limit obsahu — 20MB source pod stávajícím
  `SIZE_GATE` ho po expanzi do HTML snadno přeteče. Zamítnuto.
- `SetVirtualHostNameToFolderMapping` mapuje **celý reálný adresář** — obsah
  dokumentu by si mohl vyžádat libovolný soubor pod DocDir bez per-request
  kontroly. Zamítnuto; `WebResourceRequested` dává **per-request gating**
  (invariant 5) a je zároveň default-deny sítí (invariant 3).

Privátní host `https://mdview.invalid/` (TLD `.invalid` dle RFC 2606 nikdy
nekoliduje s reálným DNS).

#### 4.1.2 Mapování featur (FP-1 … FP-13)

| FP | Nový mechanismus | Poznámka |
|---|---|---|
| FP-1 hledání | `ICoreWebView2Find` (SDK ≥ 1.0.3405.78, Runtime ≥ 139; feature-detect přes `QueryInterface`). **Script-free fallback**: vlastníme generátor — re-render s `<mark id="m0…mN">` kolem shod nalezených v našem zdrojovém textu + navigace na `#mK`. Stávající `IDD_FIND` dialog zůstává. | Fallback pokrývá zamčené/neaktualizované runtime |
| FP-2 zoom | `ICoreWebView2Controller::put_ZoomFactor(g_zoom/100.0)`; `put_IsZoomControlEnabled(FALSE)` (perzistence zůstává naše) | 1:1 náhrada |
| FP-3 schémata | CSS variables: jeden vygenerovaný blok `:root { --md-body: …; }` na téma z nedotčené tabulky `MdThemes[]`; F9 = re-render + scroll restore (OQ-5) | 10 WCAG schémat se přenáší byte-identicky |
| FP-4 kotvy | nadpisy dostanou `id="slug"` (port sluggeru do htmlgen) → nativní `#fragment` navigace (same-document nejde přes NavigationStarting) | jednodušší než dnes |
| FP-5 .md odkazy | `NavigationStarting` → `get_Uri` → zpětné mapování `https://mdview.invalid/<relpath>` na cestu pod DocDir → `.md/.markdown` → `Cancel` + stávající `SpawnViewer` | Q3 zachováno |
| FP-6 externí | `NavigationStarting` + `NewWindowRequested` → vždy `Cancel`; allowlist http/https/mailto → `ShellExecuteW` (port dnešního gate, **bez ftp**) | OQ-6 |
| FP-7 ostatní lokální | `Cancel` + dialog s resolved path (copy-path) | beze změny chování |
| FP-8–FP-10 | beze změny (`MdDetectDecode`, `SplU8ToWExtAlloc`, `SIZE_GATE`) | mimo engine |
| FP-11 fallback | beze změny (`OpenAsText`); nově i cíl pro selhání inicializace enginu | |
| FP-12 okno/thread | zachováno; viz async init 4.1.3 | |
| FP-13 selekce | nativní Chromium selekce; Ctrl+C přes vlastní copy-only context menu / akcelerátor (plain text, invariant 8) | `put_AreDefaultContextMenusEnabled(FALSE)` + vlastní minimální menu |

**Obrázky** (dnes placeholder → plnohodnotné): relativní `src` se resolvuje
interceptorem — kanonizace, kontrola „pod DocDir, jen relativní" (invariant
5), čtení bytů, vrácení response (dekóduje engine; WIC na této cestě není
třeba). Remote `http(s)` obrázky: interceptor vrátí blokovanou odpověď +
CSS placeholder; po **per-dokument souhlasu** (D2 — infobar s názvem
hostitele) fetch provádí **handler sám přes WinHTTP** (bez cookies, limit
redirectů) — engine na síť nikdy nesahá přímo, takže consent gate je
vynutitelný na jednom místě.

**Raw HTML** (hlavní funkční zisk): md4c pass-through (`MD_FLAG_NOHTML`
vypnout) → engine renderuje nativně. Bezpečnost neřeší sanitizer (historicky
nejslabší článek — mXSS, parser-differentials; security.md:72-75, 153-154),
ale **lockdown enginu** (4.1.5): skripty vypnuté, event handlery se nikdy
nespustí, síť default-deny, formuláře/meta-refresh zrušené navigací. To je
podstata amendmentu Q1+Q2: invarianty se nepřestávají vynucovat, mění se
mechanismus z „by construction" na „by configuration + test".

#### 4.1.3 Asynchronní inicializace v CViewerThread

WebView2 je asynchronní COM (STA): `CreateCoreWebView2EnvironmentWithOptions`
→ callback → `CreateCoreWebView2Controller` → callback. Callbacky potřebují
**pumpující message loop** — ale dnešní `CViewerThread::Body` volá
`OpenFile(Name, FALSE)` **před** vstupem do `GetMessage` smyčky
(viewer.cpp:133). Nutná restrukturalizace:

1. `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` na viewer threadu.
2. `WM_CREATE` odstartuje environment→controller chain.
3. `OpenFile` jen uloží stav (cesta, dekódovaný text) a nastaví
   „pending render" flag.
4. Controller-completed handler: aplikuje lockdown (4.1.5), zaregistruje
   eventy, provede první navigaci.

Model **jeden environment + controller na viewer thread** (WebView2 objekty
jsou thread-afinní; víc environmentů nad týmž user-data folderem je
povoleno, jsou-li options identické). Alternativa „jeden sdílený WebView2
thread" zamítnuta — zbytečný zásah do osvědčeného thread-per-window modelu.

**User-data folder**: `%LOCALAPPDATA%\Open Salamander\mdview.WebView2\`
(nikdy vedle `.spl` — Program Files je read-only). Ponechává se mezi běhy
(startup cache), maže jen uninstaller.

#### 4.1.4 Akcelerátory

Fokus klávesnice žije uvnitř WebView2 HWND → `TranslateAccelerator` ve
smyčce threadu (viewer.cpp:137) Ctrl+F/F3/F9/Ctrl+U/Esc **neuvidí**. Řešení:

- `ICoreWebView2Controller::add_AcceleratorKeyPressed` → routing na stejné
  `CM_*` command ID (tabulka akcelerátorů zůstává zdrojem pravdy),
- `put_AreBrowserAcceleratorKeysEnabled(FALSE)` — zabíjí vestavěné Ctrl+P,
  Ctrl+F, F12 apod. enginu.

#### 4.1.5 Lockdown checklist — per-invariant enforcement (požadavek security.md:106)

| Inv. | Mechanismus vynucení | Třída | Re-verifikační test |
|---|---|---|---|
| 1 žádný skript | `ICoreWebView2Settings::put_IsScriptEnabled(FALSE)`; `WebMessageReceived` se nikdy neregistruje; žádné `ExecuteScript` v kódu | konfigurace | fixture s `<script>`, `onerror=`, `javascript:` URI + assert žádný efekt |
| 2 žádné spouštění | `NavigationStarting`/`NewWindowRequested` → vždy Cancel; jediné spuštění = náš `ShellExecuteW` po gestu (FP-6) | konstrukce (náš kód) | fixture s `file:`, custom schémata |
| 3 žádná síť | `AddWebResourceRequestedFilter("*", ALL)` + **default-deny** handler (povoleno jen `mdview.invalid`); `put_AdditionalBrowserArguments("--disable-background-networking")`; `put_IsReputationCheckingRequired(FALSE)` (SmartScreen); autofill/password off | konfigurace | fixture s `<img src=http://…>`, `<link>`, prefetch; síťový monitor v testu |
| 4 žádné aktivní elementy | skripty off (inv. 1) → event handlery se nespustí; formuláře/meta-refresh → navigace → Cancel; iframe subresources → tentýž deny-net; `put_AreDevToolsEnabled(FALSE)`, default context menu off, `put_IsStatusBarEnabled(FALSE)`, `put_IsBuiltInErrorPageEnabled(FALSE)` | konfigurace | fixture s formulářem, meta-refresh, iframe |
| 5 lokální zdroje | interceptor: kanonizace, prefix-check pod DocDir, read-only, odmítnutí absolute/UNC/device/`file:` | konstrukce (náš kód) | path-traversal fixtures (`..%2f`, UNC, `file:`) |
| 6 odkazy | gesto + allowlist http/https/mailto v `NavigationStarting` handleru (bez ftp) | konstrukce (náš kód) | fixture všech schémat |
| 7 robustnost | parsing+layout **out-of-process** v sandboxovaném, průběžně fuzzovaném Chromium rendereru; pád rendereru ≠ pád Salamandera (`ProcessFailed` event → error stav + OpenAsText); naše caps (size gate, md4c linear-time) před enginem | architektura (zisk!) | pathologické fixtures; `ProcessFailed` simulace |
| 8 clipboard | vlastní copy-only menu/akcelerátor, plain text | konstrukce | — |
| 9 SVG | renderuje Blink se skripty off + fetch-deny → „statický, script-free, non-fetching" splněno konfigurací | konfigurace | SVG se skriptem/external ref fixture |
| 10 re-apply | settings jsou per-controller (žijí přes navigace); handlery registrované jednou; test po reloadu | konfigurace | reload fixture |

**Re-verifikační strategie** (auto-update enginu): debug build obsahuje
self-test dokument (hook D10 existuje ve specu) s adversarial korpusem
(script/beacon/scheme sondy) spouštěný při bumpu verze runtime; init
kontroluje minimální verzi runtime (`GetAvailableCoreWebView2BrowserVersionString`),
pod minimem → failure UX (OQ-8).

**Poctivý residual risk**: samotný runtime není smluvně network-silent —
updater/telemetrie jsou OS-level služby mimo rámec aplikace. Argument:
invariant 3 governuje **content-triggered** I/O; provoz OS komponenty je
mimo jeho scope (stejně jako provoz Windows Update). Zaznamenat, nezastírat.

#### 4.1.6 Selhání a cena

**Failure modes**: environment creation selže (runtime zakázán policy,
poškozen, pod minimální verzí) → in-window error stav + `OpenAsText`
hand-off (viewer.cpp:537). Na Win11-only targetu je absence runtime téměř
vyloučená (Evergreen je součást OS servisovaná Windows Update). RTF/RichEdit
cestu **neponechávat** jako druhý backend (mrtvý kód, dvojnásobná testovací
matice) — fallbackem je interní textový viewer (OQ-4).

**Cena**: strom procesů msedgewebview2.exe (browser/GPU/renderer/utility),
~100–200 MB working set pro první okno, přírůstkově méně pro další; cold
start prvního okna ~300–600 ms (cíl: first paint < 500 ms; engine se
inicializuje až při otevření vieweru, panel Salamandera nezatěžuje).
Srovnání: RichEdit je in-process, jednotky MB, okamžitý. Toto je cena
fidelity — na moderním stroji s Win11 přijatelná pro viewer okno.

#### 4.1.7 Vendoring a licence

- **SDK** (hlavičky + statický loader) je **BSD-3-Clause** (ověřeno 07/2026)
  → GPLv2-kompatibilní; notice do `doc\third_party.txt` (stanza
  `=== Microsoft Corporation ===`). Nupkg je zip — jednorázová extrakce
  vývojářem, soubory committed; build nikdy nic nestahuje.
- **Runtime** nedistribuujeme vůbec — je to OS komponenta Windows 11
  (GPLv2 §3 „system library" argument; stejný vztah jako k user32.dll).
- Přesné soubory a úpravy projektu: sekce 7.

### 4.2 Varianta B — md4c → HTML → litehtml (contingency, 100 % in-tree)

Stejná Fáze 1 (md4c + htmlgen + CSS, sekce 5); HTML parsuje bundlovaný
gumbo, layout dělá litehtml (BSD-3, C++17, používá ho Qt Assistant — Qt 6
bundluje litehtml 0.9), **kreslí naše implementace** `document_container`
do stávajícího child-window slotu.

**Rozsah `document_container`** (~20 čistě virtuálních metod, odhad
900–1 500 LOC): `create_font/delete_font/text_width/draw_text` (nejprve GDI
+ `HFONT` cache — odpovídá kódové bázi; DirectWrite upgrade později),
`pt_to_px` (DPI), `draw_solid_fill/draw_borders/draw_list_marker`,
`load_image/get_image_size` (WIC dle precedentu `wicengine.cpp` + nanosvg
pro SVG; consent gate identický s A, jen v containeru), `import_css`
(servírovat výhradně náš stylesheet — content-provided externí CSS odmítnout),
`on_anchor_click` (port dnešního link gate téměř beze změny), `set_cursor`,
`get_media_features`, clipping.

**Subsystémy, které litehtml neposkytuje** (ověřeno upstream) — dopsat:

| Subsystém | Odhad | Poznámka |
|---|---|---|
| Scroll (WM_VSCROLL/kolečko, virtual height, offset+clip) | 300–400 LOC | přímočaré |
| **Selekce + Ctrl+C** | **800–1 500 LOC** | hit-testing bod→element→text offset, range model přes elementy, kreslení zvýraznění, extrakce plain textu. **Největší riziko** — FR-073 mandatuje „viewer-grade selection" a dotáhnout nativní pocit je těžké |
| Hledání | 300–500 LOC | vlastníme zdrojový text; `<mark>`-injection re-render + scroll-to-element |
| Zoom | malé | re-render s přeškálovaným rem základem + zachování scroll ratio (vzor `SelectScheme`, viewer.cpp:480) |

**Výhody**: 100 % samonosný build (vše ve zdrojácích), žádné procesy navíc,
identické chování offline/na zamčených systémech; skutečné tabulky, okraje,
obrázky, převážně i raw HTML; **invarianty 1/2/4 platí by construction**
(v binárce žádný script engine neexistuje) → amendment jen Q1, Q2 zůstává.

**Rizika**: (a) CSS fidelity — CSS2 + část CSS3; složitější HTML v dokumentech
se vykreslí hůř než v Chromiu; (b) údržba vendorovaného enginu — ~60 .cpp
litehtml + ~25 C souborů gumbo (největší dep po libssh2); (c) **invariant 7
obrácené znaménko**: parsing/layout nepřátelského HTML/CSS běží in-process
s plnými právy uživatele, bez druhé obranné linie (security.md:159-160 —
argument „No OS sandbox" tíží B stejně jako dřív RichEdit, zatímco A jej
řeší architektonicky); (d) **gumbo je Apache-2.0** — dle FSF neslučitelné
s čistým GPLv2, ale repo je `GPL-2.0-or-later` → kombinace konvejovatelná
pod GPLv3; konstituce („all dependencies MUST be GPLv2-compatible") vyžaduje
owner ruling nebo upřesnění formulace (OQ-3); (e) kvalita selekce vs.
mandát FR-073.

**Build**: čistý Pattern B (sekce 7), C++17 požadavek pokrývá
`stdcpplatest`.

### 4.3 Varianta C — evoluce RTF/RichEdit (baseline; proč plateau)

Fér přiznat, kam až RichEdit 4.1 (msftedit, už načtený viewer.cpp:30) umí
dojít za ~800–1 200 LOC RTF instalatérství:

- skutečné tabulkové mřížky `\trowd\cellx` + per-cell zarovnání (nahradí
  `emitTable`),
- okraje/reading measure přes `EM_SETRECT` inset,
- inline obrázky přes `EM_INSERTIMAGE` / `RICHEDIT_IMAGE_PARAMETERS`
  (Win8+ msftedit, bez OLE callback muk), plněné z WIC precedentu.

Tím se zavřou tabulky/okraje/obrázky. **Trvalé plateau**: raw HTML nikdy;
CSS-kvalita themingu (barvy rámečků, spacing) ne; typografie hrubá; každý
budoucí požadavek (mermaid, matematika, PDF export, TOC) nemožný. Verdikt:
legitimní nízkonákladový mezikrok, pokud se přepis odloží; není to cíl.

### 4.4 Varianta D — MSHTML/IWebBrowser2 (reject)

Mrtvý engine zamrzlý na IE11 sémantice, bez izolace, ActiveX-éra povrchu;
prohlášen za mrtvý v `architecture/10-plugin-maintenance-outlook.md` i
`analysis/integration.md:116`. Odstraněný plugin ieviewer
(`git ce42e70~1:src/plugins/ieviewer/`) je cenný jen jako **historický
precedent tvaru řešení**: (a) ruční OLE in-place embedding browser
kontrolu bez ATL, (b) bundlovaný cmark-gfm (Markdown→HTML) — tj. ani
embedding enginu, ani MD→HTML konverze nejsou pro tuto kódovou bázi
novinka. Reject.

---

## 5. Sdílená architektura — Fáze 1 (engine-agnostic, ~60 % práce)

Klíčové de-riskování: většina práce je společná variantám A i B a dává
hodnotu i sama o sobě (výměna parseru je už zdokumentovaný dluh —
`render.h:5-7`, `IMPLEMENTATION_NOTES.md` deviace #1).

### 5.1 md4c jako parser

- Vendor `src\common\dep\md4c\{md4c.h, md4c.c, LICENSE.md}` (MIT, jeden
  .c/.h, CommonMark 0.31, linear-time — splňuje požadavek security.md:149).
- Flagy: `MD_DIALECT_GITHUB` (tabulky, strikethrough, task listy,
  permissive autolinks); `MD_FLAG_NOHTML` = přepínač raw-HTML politiky
  (vypnout až s amendmentem Q1).
- md4c je UTF-8 nativní → pipeline `MdDetectDecode` → UTF-16 → UTF-8
  (fast-path: originální bytes, jsou-li už validní UTF-8).
- YAML front-matter md4c neumí → stávající pre-pass strip zůstává.
- Nahrazuje ~500 řádků ručního parseru v `render.cpp`.

### 5.2 htmlgen.cpp — vlastní MD_PARSER callback renderer (~600–900 LOC)

**Ne** stock `md4c-html.c` — potřebujeme:

- `id="slug"` na nadpisech (port GitHub sluggeru z `MdSlug`, percent-encoding),
- `HighlightCode` runy → `<span class="hl-kw|hl-str|…">` (adaptér
  `MDCF_*` → CSS třídy, oprava faktu 2.4/1),
- consent placeholdery pro remote obrázky, klasifikační atributy odkazů
  (interní kotva / .md / externí / blokované),
- task-list checkboxy (`disabled`),
- caps na zanoření/výstup (sémantika `MdRenderLimits`, invariant 7),
- přepínač NOHTML.

### 5.3 theme_css.cpp

Statický base stylesheet (typografie, tabulkové rámečky, code bloky,
blockquote akcent, **reading measure max-width ~900 px** + full-width
toggle dle D9) + vygenerovaný `:root { --md-…: #rrggbb; }` blok
z **nedotčené** tabulky `MdThemes[]` — 10 WCAG-korigovaných schémat se
přenáší beze změny hodnot.

### 5.4 Seam IRenderSurface

Malé rozhraní (`Create/SetHtml/SetZoom/Find/GetSelectionText/ScrollState`),
za kterým se v Fázi 2 vymění engine. Umožňuje:

- Fázi 1 dodat s **golden-file HTML testy** (vstupní .md fixtures →
  očekávané HTML) dřív, než existuje jakýkoli engine kód,
- držet konstituci III (inkrementální, revertovatelné kroky),
- kdykoli přepnout A ↔ B, pokud owner rozhodne jinak.

### 5.5 Co přežije beze změny / co umře

**Přežije**: `mdview.cpp` celý (entry, config registry klíče, `Connect`/
`AddViewer`, broadcast změn konfigurace), `CViewerThread` + lock handshake +
`SpawnViewer`, `CanViewFile` sniff, menu + tabulka akcelerátorů (s adaptací
4.1.4 u varianty A), `EffectiveTheme`/follow-system, `MdDetectDecode`,
20MB gate, `OpenAsText`.

**Umře**: RTF emise (~500 ř. `render.cpp`), `StreamInCb`/`EM_STREAMIN`/
CHARRANGE link mašinerie/`DoFind` vnitřek (~250 ř. `viewer.cpp`).

---

## 6. Doporučení

**Doporučuji variantu A (WebView2), fázovaně, s variantou B jako
zaznamenanou contingency.** Argumentace:

1. **Cíle uživatele**: tabulky/okraje/typografii dají A i B; nativní raw
   HTML plně jen A; **budoucí rozšiřitelnost (mermaid, matematika, export
   do PDF přes `PrintToPdf`, bohatší UI pohledu) je reálná jen s A** —
   u B by každá taková featura znamenala vendorovat další renderer.
2. **Konstituce**: princip IV (Windows 11+ commitment) dělá z Evergreen
   runtime **garantovanou OS komponentu** — nejsilnější jednotlivý argument
   pro A; princip I splněn — vše, co se kompiluje nebo linkuje, je v repu
   (sekce 7), build nic nestahuje; princip III splněn fázováním; princip VI
   nedotčen (WebView2 kreslí jen do vlastního HWND).
3. **Bezpečnost — oboustranně poctivě**: A mění invarianty 1/2/4
   z „by construction" na „by configuration + test" (tabulka 4.1.5 + D10
   re-verifikace je odpověď na security.md:98-107); výměnou získává
   **architektonické zpevnění invariantu 7** — parsing nepřátelského obsahu
   běží out-of-process v sandboxovaném, průběžně fuzzovaném Chromium
   rendereru, zatímco B (i dnešní stav) parsuje in-process s plnými právy.
4. **Čtyři historické blokery z integration.md:112-115 vyřešeny**:
   (1) *NuGet-free vendoring* — nupkg je zip, jednorázová extrakce, soubory
   committed; (2) *licence loaderu* — SDK je BSD-3-Clause, notice do
   `doc\third_party.txt`, runtime nedistribuujeme (GPLv2 §3 system
   library); (3) *JS-off proof* — lockdown 4.1.5 + adversarial self-test
   korpus + re-verifikační procedura při bumpu runtime; (4) *child
   procesy* — akceptovaná, kvantifikovaná cena (~100–200 MB, engine se
   spouští až s viewer oknem), ospravedlnitelná na Win11-only targetu.
5. **Governance**: vyžaduje amendment Q1+Q2 (sekce 9). Pokud owner
   statický-renderer ruling podrží, **Fáze 1 se nemění a nastupuje
   varianta B** — to je praktická hodnota fázování: rozhodnutí o enginu
   se dá odložit až za Fázi 1 bez ztráty práce.

### Fázový plán

| Fáze | Obsah | Viditelná změna |
|---|---|---|
| **P1** | md4c + htmlgen + theme_css + IRenderSurface + golden-file testy | žádná (RTF backend dočasně krmen z nového modelu, nebo P1 čistě přípravná větev) |
| **P2** | Engine swap: WebView2 bootstrap (async init, user-data folder), lockdown 4.1.5, mapování FP-1…FP-13, Find + fallback | tabulky, okraje, typografie, raw HTML |
| **P3** | Obrázky + consent UI (D2), selekce/copy polish, self-test korpus (D10) | inline obrázky |
| **P4** | Extras: mermaid, matematika, PrintToPdf export, TOC… | mimo scope této analýzy (enabled-by) |

---

## 7. Build & vendoring appendix

Zásada: **nula změn** v `salamand.sln`, `salamand.gen.slnf`, `plugins.cfg`
(řádek `mdview=on` existuje), `build.cmd`. Vše se odehraje v
`mdview.vcxproj` + `mdview.props` podle přesného precedentu libssh2→sftp
(`sftp.vcxproj:119-148`: `<ClCompile>` s per-file `<ObjectFileName>` +
`<PrecompiledHeader>NotUsing</PrecompiledHeader>`; include/defines/libs
v plugin-lokálním props).

### 7.1 Společné (md4c) — Fáze 1

```
src\common\dep\md4c\
  md4c.h
  md4c.c
  LICENSE.md
```

`mdview.vcxproj`:

```xml
<ClCompile Include="..\..\..\common\dep\md4c\md4c.c">
  <ObjectFileName>$(IntDir)lib_md4c.obj</ObjectFileName>
  <PrecompiledHeader>NotUsing</PrecompiledHeader>
</ClCompile>
```

`mdview.props`: přidat `..\..\..\common\dep\md4c` do
`AdditionalIncludeDirectories`.

### 7.2 Varianta A (WebView2) — Fáze 2

```
src\common\dep\webview2\
  include\WebView2.h
  include\WebView2EnvironmentOptions.h
  lib\x86\WebView2LoaderStatic.lib
  lib\x64\WebView2LoaderStatic.lib
  LICENSE.txt          (BSD-3-Clause text z nupkg)
  VERSION.txt          (pinned verze SDK, např. 1.0.3405.78+)
```

Získání: `Microsoft.Web.WebView2` nupkg = zip; jednorázová extrakce
vývojářem, commit do repa. ARM64 lib se nevendoruje (solution nemá ARM64
konfigurace). Build stroje nikdy nic nestahují.

`mdview.props`:

- `AdditionalIncludeDirectories` += `..\..\..\common\dep\webview2\include`
  (+ `..\..\..\common\dep` pro `wil/com.h` — `wil::com_ptr` na COM RAII,
  precedent tserver),
- `AdditionalLibraryDirectories` += `..\..\..\common\dep\webview2\lib\$(ShortPlatform)`
  (macro `x86`/`x64` z `x86.props`/`x64.props` — pokrývá obě architektury),
- `AdditionalDependencies` += `WebView2LoaderStatic.lib;version.lib`
  (+ `ole32.lib`, není-li tranzitivně).

CRT poznámka (OQ-7): ověřit spike-em link statického loaderu proti /MD(d);
fallback = `WebView2Loader.dll` kopírovaná automatizovaným PostBuild
krokem do output adresáře pluginu (a v Release podepsaná stávajícím
codesign krokem).

`doc\third_party.txt`: nová stanza `=== Microsoft Corporation ===`
s BSD-3 textem WebView2 SDK.

### 7.3 Varianta B (litehtml) — jen pokud padne volba B

```
src\common\dep\litehtml\
  include\**            (public hlavičky)
  src\*.cpp             (~60 souborů)
  src\gumbo\**          (~25 C souborů + hlavičky)
  LICENSE               (BSD-3) + gumbo LICENSE (Apache-2.0!)
```

Pinnout na tagovaný release; ~85 `<ClCompile>` položek se vzorem
`<ObjectFileName>$(IntDir)lib_litehtml_%(Filename).obj</ObjectFileName>`
(zabrání kolizím v plochém `$(IntDir)`), gumbo kompilovat jako C, vše
`NotUsing` PCH. Zaznamenat rozhodnutí o `LITEHTML_UTF8`. Nová stanza
v `doc\third_party.txt` (litehtml BSD-3 + gumbo Apache-2.0 — viz OQ-3).

### 7.4 Důkaz konstituce I

Čistý stroj + VS2022 + `git clone` + `build.cmd` → kompletní build bez
síťového přístupu. Platí pro 7.1 i 7.2 (binární .lib je committed artefakt
s pinned verzí a licencí, stejná třída jako convert tabulky).

---

## 8. Rizika a otevřené otázky

### 8.1 Rizika (nad rámec per-variant rizik v sekci 4)

| Riziko | Dopad | Mitigace |
|---|---|---|
| Scroll restore přes re-navigaci (změna schématu/zoomu u A) | UX drhnutí | OQ-5; fragment/scroll ratio, změřit ve spike |
| RTL/bidi, IME chování jiné než RichEdit | okrajové regresní chování | testovací fixtures |
| Find API skew (Runtime < 139 na neaktualizovaných strojích) | horší hledání | feature-detect + `<mark>` fallback (4.1.2) |
| „Instant open" pocit (RichEdit je okamžitý) | vnímaná regrese | budget first paint < 500 ms; lazy engine init; měřit v P2 |
| Multiplicita environmentů (1/viewer thread) | RAM při mnoha oknech | sdílený user-data folder; changelog limit oken ne— dokumentovat |
| Kvalita selekce u B vs. FR-073 | nesplnění mandátu | hlavní důvod, proč B je contingency, ne primární volba |

### 8.2 Otevřené otázky pro ownera

- **OQ-1** *(gating)*: Amendment Q2 — povolit browser engine (WebView2)?
  Bez něj platí varianta B.
- **OQ-2**: Amendment Q1 — renderovat raw HTML nativně? Doporučení: ano,
  **bez sanitizeru** (spolehnout na engine lockdown; sanitizery jsou
  historicky nejslabší článek, security.md:72-75).
- **OQ-3** *(jen B)*: gumbo Apache-2.0 vs. konstituce „GPLv2-compatible
  deps" — repo je GPL-2.0-or-later, kombinace konvejovatelná pod GPLv3;
  potvrdit/upřesnit formulaci konstituce.
- **OQ-4**: Po swapu smazat RTF cestu (doporučení: ano — fallback je
  OpenAsText, dvojí backend = mrtvý kód + dvojí testovací matice)?
- **OQ-5**: Mechanismus přepnutí schématu u A: re-navigace + scroll restore
  (jednoduché, poctivé bez JS) vs. výměna stylesheetu druhým resource
  fetchem. Rozhodnout ve spike.
- **OQ-6**: Odstranit `ftp` z allowlistu odkazů (deviace od invariantu 6)?
  Doporučení: ano.
- **OQ-7**: CRT kompatibilita `WebView2LoaderStatic.lib` s /MD(d) — ověřit
  spike-em; fallback `WebView2Loader.dll` + PostBuild copy + codesign.
- **OQ-8**: Politika minimální verze runtime + failure UX (error stav
  s vysvětlením vs. tichý OpenAsText).
- **OQ-9**: `mdview.props` dědí boilerplate `WINVER=0x0601` (Win7 éra) —
  zvednout repo-konzistentně (samostatný úklid, netýká se jen mdview).

---

## 9. Návrh textu amendmentů (k ratifikaci ownerem — spec.md se touto analýzou nemění)

**Amendment Q2 (FR-046)** — návrh znění do Decisions Log:

> *2026-XX-XX — Q2 amended: mdview v2 přechází na md4c → HTML pipeline
> vykreslovanou přes WebView2 (Evergreen Runtime, OS komponenta Windows 11).
> Bezpečnostní invarianty 1–10 zůstávají nepodkročitelné; mění se třída
> vynucení z „by construction" na „by configuration + test" dle
> per-invariant enforcement tabulky (analysis/html-renderer.md §4.1.5)
> s re-verifikační procedurou při každém bumpu verze runtime (D10 self-test
> korpus). Fallback při nedostupnosti runtime: interní textový viewer.*

**Amendment Q1 (FR-015)** — návrh znění:

> *2026-XX-XX — Q1 amended: raw HTML bloky a inline HTML se vykreslují
> nativně HTML enginem (bez sanitizeru). Invarianty 1/3/4 vynucuje lockdown
> enginu (skripty vypnuté, síť default-deny, navigace Cancel-all); event
> handlery se nikdy nespustí, aktivní elementy jsou neaktivní. Mikro-whitelist
> z původního Q1 se stává bezpředmětným (nadmnožina).*

Pozn.: pokud OQ-1 dopadne „ne" (owner podrží statický renderer), amendment
Q2 se neprovádí a Q1 se přeformuluje pro litehtml (statický script-free
renderer zůstává, mění se jen HTML politika).

---

## Zdroje

- [WebView2: Evergreen vs. fixed — Runtime předinstalován na Windows 11](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/evergreen-vs-fixed-version)
- [WebView2: Distribute your app and the WebView2 Runtime](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution)
- [WebView2: Distribute as single executable (WebView2LoaderStatic)](https://learn.microsoft.com/en-us/microsoft-edge/webview2/how-to/static)
- [WebView2 SDK release notes (Find API stabilní v 1.0.3405.78, Runtime 139+)](https://learn.microsoft.com/en-us/microsoft-edge/webview2/release-notes/)
- [Microsoft.Web.WebView2 na NuGet (BSD-3-Clause licence SDK)](https://www.nuget.org/packages/Microsoft.Web.WebView2)
- [litehtml — GitHub (BSD-3-Clause, C++17, bundlovaný gumbo)](https://github.com/litehtml/litehtml)
- [litehtml issue #66 — selekce textu je app-side](https://github.com/litehtml/litehtml/issues/66)
- [Qt Assistant attribution — litehtml 0.9](https://doc.qt.io/qt-6/qtassistant-attribution-litehtml.html)
- [md4c — GitHub (MIT, CommonMark 0.31, GFM flagy)](https://github.com/mity/md4c)
