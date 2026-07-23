# Dotazník rozhodnutí — Newt Commander Rebrand (feature 032)

**Účel**: Zachytit všechna rozhodnutí, která je vhodné učinit ve specify/clarify fázi.
Každá položka uvádí kontext zjištěný z kódu, varianty a doporučení (výchozí volbu).
Položky označené **[KRITICKÉ — Qn]** blokují plánování a jsou zrcadleny jako
`[NEEDS CLARIFICATION]` ve spec.md. Ostatní položky mají rozumné výchozí hodnoty
zapsané v sekci Assumptions ve spec.md — pokud je neodpovíte, platí doporučení.

**Jak odpovědět**: stačí stručně, např. `Q1: A, Q2: A, Q3: B; D01: B, D07: A, zbytek dle doporučení`.

---

## A. Identita produktu a pojmenování

### D01 — Kořen registru pro konfiguraci
- **Kontext**: Dnes `HKCU\Software\Open Salamander\5.0` (`src/mainwnd2.cpp:159`), s řetězem ~20 legacy
  kořenů (`Software\Altap\…`, `Software\Salamander`) pro auto-import.
- **Varianty**:
  - **A (doporučeno)**: `HKCU\Software\Newt Commander\0.1` — zachovává strukturu „výrobce\verze",
    minimální zásah do logiky, budoucí verze mohou migrovat mezi podklíči.
  - **B**: `HKCU\Software\Newt Commander` bez verzového podklíče (verze config schématu jako hodnota).
  - **C**: `HKCU\Software\NewtCommander\…` (bez mezery — snazší pro skripty, méně vzhledné v regeditu).
- **Dopad**: Ovlivňuje i konfiguraci všech pluginů (dostávají podklíč od aplikace).

### D02 — Vydavatel / CompanyName ve version info
- **Kontext**: Dnes `VERSINFO_COMPANY "Open Salamander"` (`src/versinfo.rh2`).
- **Varianty**: **A (doporučeno)**: „Newt Commander Project" · **B**: „Newt Commander" · **C**: osobní jméno autora.

### D03 — Copyright řádek (About, file properties, splash)
- **Kontext**: Dnes „Copyright © 1997-2023 Open Salamander Authors". GPLv2 vyžaduje zachování
  původních copyright notices; About dialog je přirozené místo pro atribuci.
- **✅ ROZHODNUTO (clarify 2026-07-23)**: Roční dělení — do roku 2026 „Open Salamander Authors",
  od 2026 dále „Newt Commander Authors" (tj. „© 1997–2026 Open Salamander Authors ·
  © 2026 Newt Commander Authors"). Nové pluginy (SFTP, Markdown Viewer) pouze
  „© Newt Commander Authors"; PictView (upraven tímto projektem) duální atribuce.
- **Poznámka**: Texty licencí v `doc/` zůstávají beze změny.

### D04 — Oficiální URL projektu
- **Kontext**: UI dnes odkazuje na `www.altap.cz`, `forum.altap.cz` (About `src/logo.cpp:424`,
  menu Help `src/mainwnd3.cpp:2565,2571`, dialogy `src/dialogs.cpp:2078`, versioninfo `src/versinfo.rh2:34`).
  Git remote je `https://github.com/newtcommander/newtcommander`.
- **✅ ROZHODNUTO (clarify 2026-07-23)**: Hlavní web `https://newtcommander.org`, repozitář
  `https://github.com/newtcommander/newtcommander`; odkazy bez ekvivalentu se odstraní.

### D05 — Názvy pomocných binárek
- **Kontext**: Vedle hlavního exe existují `salmon.exe` (crash reporter), `salextx86/x64.dll`
  (shell extension), `english.slg`, pluginy `*.spl`. Zadání explicitně žádá jen `newtcommander.exe`.
- **Varianty**:
  - **A (doporučeno)**: Přejmenovat jen hlavní exe; pomocné soubory ponechat (rebrand jen jejich
    metadata a viditelné texty). Nejmenší riziko — na názvy se odkazuje kód i registrace.
  - **B**: Přejmenovat i pomocné binárky (např. `newtmon.exe`, `newtextx64.dll`) — konzistentnější,
    ale větší zásah (odkazy v kódu, regsvr32 instrukce, build).
- **Poznámka**: `salmon.cpp:331` spouští natvrdo `salamand.exe` — po přejmenování hlavního exe se musí
  upravit v obou variantách.

### D06 — Explicitní AppUserModelID (taskbar)
- **Kontext**: Dnes žádné explicitní AppID nastaveno není (kód zakomentován, `src/jumplist.cpp:26-55`).
- **Varianty**: **A (doporučeno)**: nezavádět (zachovat současné chování, mimo rozsah) ·
  **B**: nastavit nové `NewtCommander.NewtCommander` (samostatné seskupování na taskbaru, jumplist identita).

---

## B. Verzování

### D07 — Interní build number a formát zobrazení verze
- **Kontext**: Dnes verze „5.0", build 184 (`src/plugins/shared/spl_vers.h`); titulky zobrazují
  „Open Salamander 5.0". Nová verze 0.1.0.
- **Varianty**:
  - **A (doporučeno)**: Zobrazovat „Newt Commander 0.1.0"; interní build number pokračuje (185+)
    a je vidět jen v About/file properties (0.1.0.185). Monotónie chrání logiku závislou na buildu.
  - **B**: Reset build číslování na 1 (čistý start, ale build < 184 může zmást logiku porovnávání buildů).

### D08 — Windows FileVersion/ProductVersion
- **Kontext**: Manifest dnes deklaruje 5.0.0.0; VERSIONINFO odvozeno z maker.
- **Doporučení**: ProductVersion „0.1.0", FileVersion 0.1.0.<build>. Bez variant — potvrďte.

### D09 — Označení verze konfigurace
- **Kontext**: Paralelní pole `SalamanderConfigurationVersions[]` nese label „5.0" pro aktivní kořen.
- **Doporučení**: Nový label „0.1" pro nový kořen (svázáno s D01). Bez variant — potvrďte.

### D10 — Verze plugin API (ABI)
- **Kontext**: `LAST_VERSION_OF_SALAMANDER 104` — ABI brána pro načítání pluginů; hláška při odmítnutí
  zní „This plugin requires Open Salamander 5.0 build 184 …" (`spl_vers.h:200-201`).
- **Varianty**: **A (doporučeno)**: ABI číslo 104 beze změny (pluginy z tohoto repa fungují dál),
  rebrand pouze textu hlášky na „Newt Commander 0.1.0" · **B**: povýšit ABI (zbytečný break, proti ústavě).

---

## C. Migrace a koexistence

### D11 — ~~[KRITICKÉ — Q1]~~ ✅ ROZHODNUTO (clarify 2026-07-23): varianta A — čistý start, žádný import, importní mechanismus odstranit
- **Kontext**: Aplikace dnes při prvním startu auto-importuje config z řetězu legacy kořenů
  (`src/mainwnd2.cpp:157-242`, UI `src/dialogs2.cpp:652-684`). Zadání říká „žádná zpětná kompatibilita,
  žádné sdílené registry" — ale jednorázový read-only import do vlastního kořene sdílení není.
- **Varianty**:
  - **A**: Čistý start — žádný import, importní mechanismus se z aplikace odstraní. Nejčistší oddělení;
    stávající uživatel Salamanderu si vše nastaví znovu.
  - **B**: Jednorázová nabídka read-only importu z `Software\Open Salamander\5.0` do nového kořene
    při prvním startu (legacy Altap/Servant řetěz se odstraní). Pohodlnější přechod, malé riziko
    přenosu „salamandřích" hodnot (cesty k pluginům apod. se musí přemapovat).
- **Dopad**: Značný na rozsah — varianta B přidává mapování a testování importní cesty.

### D12 — Osud kódu auto-importu legacy kořenů
- **Kontext**: I při variantě D11-A zůstává v kódu ~80 řádků legacy kořenů a importní UI.
- **Varianty**: **A (doporučeno)**: odstranit aktivní použití (seznam kořenů zredukovat na nový kořen),
  mrtvý kód smazat · **B**: jen deaktivovat (ponechat pro případ budoucí varianty B).

### D13 — Shell extension
- **Kontext**: Registruje se jako `OpenSalamanderVer500` s CLSID `{c78b614f-…}` (`src/shexreg.h:61,107`),
  sdílená paměť `SalExten_SharedMem4` atd. Stejné identifikátory = kolize s nainstalovaným Salamanderem.
- **Varianty**: **A (doporučeno)**: zachovat funkci, vygenerovat **nové CLSID**, registrace
  `NewtCommanderVer010`, nové názvy sdílených objektů, rebrand popisů ·
  **B**: shell extension v 0.1.0 vůbec neregistrovat/nešípovat (odklad funkce).

---

## D. Vizuální styl

### D14 — Chybějící assety v balíčku vizuálního stylu
- **Kontext**: `temp/visual_style/README.txt` popisuje `icon-32.svg`, `favicon.svg`, PNG rastry
  (512–16 px), vertikální lockupy a `lockup/png/` — ale v adresáři reálně jsou jen 3 SVG
  (plná ikona + 2 horizontální lockupy). Pro Windows `.ico` (16–256 px) jsou zjednodušené
  varianty nutné.
- **Varianty**: **A (doporučeno)**: chybějící varianty vytvořím v rámci implementace ze SVG dle
  pravidel v README.txt (≥48 plná, 24–48 zjednodušená, ≤16 favicon) · **B**: dodáte je vy
  (originály od designéra) — implementace na ně počká.

### D15 — Složení .ico souboru
- **Doporučení**: Velikosti 16, 24, 32, 48, 64, 128, 256 px (32bit PNG-komprimované pro ≥64 px),
  varianty dle D14. Bez variant — potvrďte.

### D16 — Tray ikony stavů (červená/zelená/modrá)
- **Kontext**: `sal_r.ico`, `sal_g.ico`, `sal_b.ico` — barevné stavové varianty hlavní ikony v tray.
- **Varianty**: **A (doporučeno)**: odvodit z nové ikony s barevným akcentem zachovávajícím
  význam stavů · **B**: stavové odlišení řešit overlay badge na nové ikoně.

### D17 — About dialog a splash — použití lockupů a témata
- **Kontext**: About (`IDD_ABOUT`, `src/logo.cpp:320+`) a splash (`IDD_SPLASH`) dnes kreslí SVG
  (`res/logo.svg`, `res/os.svg`, gradienty). Aplikace má od feature 028 světlé/tmavé téma;
  balíček má lockup dark + light.
- **Varianty**: **A (doporučeno)**: About i splash respektují aktivní téma (dark lockup na tmavém,
  light na světlém; ikona je univerzální) · **B**: jednotný tmavý „brand" vzhled (navy pozadí
  #0A1424) nezávisle na tématu — odpovídá plakátovému stylu balíčku.

### D18 — Písmo Archivo v lockupech
- **Kontext**: Text v SVG lockupech je živý text vyžadující nainstalované Archivo (jinak spadne na Arial).
- **Varianty**: **A (doporučeno)**: před zabudováním do zdrojů převést texty na křivky (příp. rastr);
  žádná instalace/embedding fontu do aplikace · **B**: vykreslit název programově stávajícími
  prostředky aplikace (bez závislosti na SVG textu).

### D19 — Splash screen
- **Varianty**: **A (doporučeno)**: zachovat splash s novým vizuálem · **B**: splash zrušit
  (rychlejší start, méně assetů) — drobná změna chování oproti dnešku.

---

## E. Crash reporter (salmon)

### D20 — ~~[KRITICKÉ — Q2]~~ ✅ ROZHODNUTO (clarify 2026-07-23): varianta A — upload vypnout, dumpy jen lokálně
- **Kontext**: `salmon.exe` dnes uploaduje reporty na **`reports.altap.cz`** (`src/salmon/upload.cpp:13`)
  — server původního vendora. Po rebrandu tam data Newt Commanderu nesmí téct (soukromí + cizí server).
- **Varianty**:
  - **A (doporučeno)**: Upload vypnout; dumpy se ukládají jen lokálně do `%APPDATA%\Newt Commander`
    a uživatel je může přiložit k GitHub Issue. Nejmenší rozsah, žádná infrastruktura.
  - **B**: Vlastní endpoint (museli byste provozovat server) — infrastruktura mimo rozsah tohoto repa.
  - **C**: Salmon úplně odstranit z buildu (ztratíte minidumpy — nedoporučeno, pomáhá ladění).

---

## F. Instalátor, nápověda, překlady

### D21 — ~~[KRITICKÉ — Q3, část]~~ ✅ ROZHODNUTO (clarify 2026-07-23): varianta A — odložit
- **Kontext**: Zdroje setup/remove jsou plné „Open Salamander" řetězců; skutečná install konfigurace
  (`.inf` s názvem složky, zástupci, Uninstall DisplayName) v repu chybí — dodává se při balení.
  Instalátor se aktuálně nedistribuuje.
- **Varianty**: **A (doporučeno)**: odložit do samostatné feature (033+); v 032 jen zajistit, že se
  nic salamandřího nešípuje · **B**: rebrandovat zdroje setup/remove už teď (bez .inf ale nejde ověřit).

### D22 — ~~[KRITICKÉ — Q3, část]~~ ✅ ROZHODNUTO (clarify 2026-07-23): varianta A — odložit
- **Kontext**: Obsah nápovědy je kompletně salamandří; help menu už bylo zjednodušeno (feature 019).
- **Varianty**: **A (doporučeno)**: odložit — v 032 jen ověřit, že odkazy z aplikace nevedou na
  salamandří web · **B**: rebrand nápovědy teď (velký objem obsahu).

### D23 — ~~[KRITICKÉ — Q3, část]~~ ✅ ROZHODNUTO (clarify 2026-07-23): varianta A — jen angličtina, překlady odložit
- **Kontext**: Build standardně šípuje jen `english.slg`; překlady obsahují „Open Salamander" v tisících řetězců.
- **Varianty**: **A (doporučeno)**: v 032 jen angličtina; překlady odložit · **B**: rebrand i překladů.

---

## G. Vývojářské a procesní

### D24 — Vývojářsky viditelné názvy
- **Kontext**: `OPENSAL_BUILD_DIR`, `salamand.sln`, `salamand.vcxproj`, `plugins.cfg`, cesty buildu.
  Zadání: zdrojové soubory a interní názvy se nepřejmenovávají.
- **Doporučení**: Vše ponechat (jen výstupní název binárky se mění na `newtcommander.exe`).
  Bez variant — potvrďte.

### D25 — Dodatek ústavy projektu
- **Kontext**: Ústava (`.specify/memory/constitution.md`) se jmenuje „Open Salamander Constitution"
  a princip II garantuje nepokažení funkčnosti „Open Salamander 5.0". Tato feature kompatibilitu
  záměrně láme — „zásadní pravidlo" ze zadání je governance změna.
- **Varianty**: **A (doporučeno)**: součástí 032 je dodatek ústavy — přejmenování na „Newt Commander
  Constitution", princip II překotven na baseline Newt Commander 0.1.0 s dokumentovaným breakem
  (MAJOR bump verze ústavy) · **B**: ústavu řešit samostatně mimo tuto feature.

### D26 — Identita v CLAUDE.md a architektonických dokumentech
- **Kontext**: `CLAUDE.md` popisuje projekt jako „Open Salamander"; `architecture/*.md` též.
- **Varianty**: **A (doporučeno)**: v 032 aktualizovat CLAUDE.md (kontext pro agenty musí odpovídat
  realitě po rebrandu); `architecture/` dokumenty ponechat jako historickou analýzu s poznámkou ·
  **B**: aktualizovat vše včetně architecture/.

---

## Souhrn výchozích voleb (platí, pokud neodpovíte jinak)

| # | Výchozí volba |
|---|---------------|
| D01 | A — `HKCU\Software\Newt Commander\0.1` |
| D02 | A — „Newt Commander Project" |
| D03 | ✅ rozhodnuto — roční dělení copyrightu (do 2026 OS Authors, od 2026 NC Authors); nové pluginy jen NC Authors |
| D04 | ✅ rozhodnuto — web newtcommander.org + GitHub repo |
| D05 | A — přejmenovat jen hlavní exe |
| D06 | A — AppUserModelID nezavádět |
| D07 | A — build number pokračuje, zobrazuje se 0.1.0 |
| D08 | potvrdit — 0.1.0.<build> |
| D09 | potvrdit — config label „0.1" |
| D10 | A — ABI 104 beze změny, jen text hlášky |
| D11 | ✅ rozhodnuto — A: čistý start bez importu |
| D12 | A — legacy import kód odstranit |
| D13 | A — nové CLSID + `NewtCommanderVer010` |
| D14 | A — chybějící assety vygenerovat dle README.txt |
| D15 | potvrdit — 16–256 px .ico |
| D16 | A — stavové tray ikony odvodit z nové ikony |
| D17 | A — About/splash dle aktivního tématu |
| D18 | A — texty lockupů na křivky/rastr |
| D19 | A — splash zachovat |
| D20 | ✅ rozhodnuto — A: upload vypnut, dumpy lokálně |
| D21–D23 | ✅ rozhodnuto — A: vše odloženo, v 032 jen anglické zdroje aplikace |
| D24 | potvrdit — dev názvy beze změny |
| D25 | A — dodatek ústavy součástí 032 |
| D26 | A — aktualizovat CLAUDE.md, architecture/ s poznámkou |
