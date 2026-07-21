# Analýza: Ikony toolbaru a tmavé téma

**Feature**: 029-dark-toolbar-icons
**Datum**: 2026-07-21
**Účel**: Odpověď na zadání „analyzuj, jak a kde jsou použity SVG ikony
z adresáře `toolbars` ve výsledném buildu, a možnosti nahrazení výchozích
ikon sadou, která lépe odpovídá tmavému pozadí."

## 1. Jak ikony toolbaru fungují dnes

### SVG ikony v adresáři `toolbars` JSOU živé (načítají se za běhu)

Adresář `src\res\toolbars\` obsahuje ~63 SVG souborů (Back.svg, Copy.svg,
…). Nejsou to mrtvé/referenční assety — aplikace je **za běhu čte z disku
a rasterizuje**:

- **Kopírování do buildu**: `src\vcxproj\!populate_build_dir.cmd:115-116`
  kopíruje (robocopy) `src\res\toolbars\` do
  `<build>\salamander\<Config>\toolbars\` vedle `salamand.exe`. SVG nejsou
  součástí `.vcxproj` ani zdrojů (.rc) — jsou to čistě runtime soubory.
- **Runtime loader**: `RenderSVGImage()` v `src\svg.cpp:92` sestaví cestu
  `<adresář exe>\toolbars\<jméno>.svg` (svg.cpp:94-98), soubor načte,
  parsuje knihovnou **nanosvg** (`src\common\dep\nanosvg\`) a rasterizuje
  do 32bit DIBu, který AlphaBlendem nakreslí do cílového DC (svg.cpp:141-149).
- **Kdo to volá**: `RenderSVGImages()` (`src\toolbar4.cpp:709-718`),
  voláno z `CreateToolbarBitmaps()` (toolbar4.cpp:834) — SVG glyfy se
  **razítkují přes legacy rastrový podklad**. Mapování tlačítko→SVG je
  v tabulce `ToolBarButtons[]` (`toolbar4.cpp:149-252`, pole `SVGName`).

### Celkový postup sestavení obrázků tlačítek (start aplikace)

`CreateToolbarBitmaps()` (`src\toolbar4.cpp:730`) vytváří sdílené image
listy `HHotToolBarImageList` (barevný) a `HGrayToolBarImageList`
(disabled), definované v `salamdr1.cpp:289-290`, plněné v
`salamdr1.cpp:2400-2538`:

1. Načte legacy rastrový arch — `IDB_TOOLBAR_16` (16barevný BMP) nebo
   `IDB_TOOLBAR_256` (plnobarevné PNG `toolbar2.png`), volba podle hloubky
   obrazovky `Use256ColorsBitmap()` (`salamdr1.cpp:1785`), ne podle
   nastavení uživatele.
2. Škáluje na velikost ikony dle DPI — základ **16×16**,
   `GetIconSizeForSystemDPI(ICONSIZE_16)`.
3. **Přes podklad vykreslí SVG glyfy** (jen tlačítka, která mají SVGName
   a soubor existuje).
4. Připojí ikony ze shell32.dll (síťové/systémové příkazy).
5. Vyrobí šedou (disabled) variantu + masku
   (`CreateGrayscaleAndMaskBitmaps`, toolbar4.cpp:~570-707).

Stejné dva image listy krmí **toolbary i menu** (mainwnd1.cpp:3130-3139,
menu4.cpp:365-375, finddlg2.cpp:350-439, …).

## 2. Co udělalo rozšíření 028 (tmavé téma) — a kde je mezera

- Pixelová transformace `ThemeAdjustBitmapForDarkMode()`
  (`src\themes.cpp:373-467`): v tmavém režimu invertuje tmavost
  neutrálních pixelů (černé kontury → světlé) a zesvětluje tmavé syté
  barvy; průhlednost zachovává.
- Aplikuje se ale **jen na legacy rastrový arch, PŘED vykreslením SVG**
  (`toolbar4.cpp:770`; komentář to výslovně říká). Pozadí image listů už
  tmavé je (`ImageList_SetBkColor(..., ThemeSysColor(COLOR_BTNFACE))`,
  salamdr1.cpp:2537).
- **Mezera**: SVG glyfy se v enabled stavu kreslí v původních (světlému
  tématu určených) barvách — recolor existuje jen pro disabled stav
  (svg.cpp:128-138). Proto řada ikon na tmavém pozadí „nefunguje":
  tmavé kontury a výplně zapadají do tmavého podkladu.
- Infrastruktura pro obarvení SVG podle tématu už existuje:
  `CSVGSprite::ColorizeSVG` (`svg.cpp:329-356`) — dnes se používá jen pro
  šipkové sprity (dropdown/more/less), obarvuje tvary na
  `ThemeSysColor(COLOR_BTNTEXT/BTNSHADOW)`.

### Vedlejší nálezy (kandidáti na opravu v rámci 029)

- **Překlep v assetu**: tabulka odkazuje `"ClipboardCut"`, soubor se ale
  jmenuje `CilpboardCut.svg` — glyf tiše padá na legacy rastr
  (toolbar4.cpp:182 vs. src\res\toolbars\CilpboardCut.svg).
- **Neúplné pokrytí**: řada `SVGName` v tabulce nemá soubor vůbec
  (Select/Unselect/InvertSelection/SelectAll/…, New_Insert, …) → tato
  tlačítka jedou čistě z legacy rastru (ten ale dark transform dostává).
- **Ikony pluginů** (`CPlugins::CreateIconsList`, plugins2.cpp:984-1010)
  žádnou tmavou úpravu nedostávají — mimo rozsah 029, samostatné téma.

## 3. Možnosti řešení „tmavé sady ikon"

| # | Varianta | Princip | Pro | Proti |
|---|----------|---------|-----|-------|
| A | Transformace po kompozici | Přesunout/rozšířit `ThemeAdjustBitmapForDarkMode` tak, aby běžela až **po** vykreslení SVG (a příp. mimo shell ikony) | Nejméně práce, pokryje SVG i legacy najednou, žádné nové assety | Heuristika může zkreslit vícebarevné glyfy; shell ikony nutno vynechat (pořadí kompozice) |
| B | Recolor při rasterizaci SVG | V `RenderSVGImage` v tmavém režimu selektivně obarvit tmavé/neutrální tahy (obdoba `ColorizeSVG`, práh jasu jako v pixelové heuristice) | Vektorová kvalita, cílené (barevné akcenty lze zachovat), žádné nové assety | Jen pro SVG tlačítka; legacy rastr řeší stávající transform; ladění prahů |
| C | Druhá sada tmavých SVG | Dodat `toolbars\dark\*.svg` (nebo sufix `_dark`), za běhu vybírat podle aktivního tématu, fallback na světlé SVG + recolor | Plná výtvarná kontrola per ikona; nejlepší možný výsledek | ~63 nových assetů k výrobě a údržbě; riziko nekonzistence se zbylými legacy glyfy |
| D | Nová jednotná monochromatická sada | Outline styl obarvovaný `COLOR_BTNTEXT` v obou tématech (moderní vzhled à la VS Code/Windows 11) | Nejkonzistentnější; theming zadarmo navždy | Největší práce; mění vzhled i výchozího tématu → koliduje s požadavkem „Default beze změny" |

## 4. Doporučení (fázované)

1. **P1 = B + A**: v tmavém režimu obarvovat SVG glyfy při rasterizaci
   (B) a ponechat/doladit stávající pixelovou transformaci pro legacy
   rastr (A jen v míře nutné pro tlačítka bez SVG). Pokrývá 100 %
   tlačítek bez jediného nového assetu; Default zůstává beze změny.
2. **P2 = C jako cílená nadstavba**: pro ikony, kde automatika dopadne
   špatně (vícebarevné motivy), umožnit dodání ručně laděné tmavé SVG
   varianty s runtime výběrem dle tématu. Mechanismus stačí zavést,
   assety lze doplňovat postupně.
3. **Opravit překlep** `CilpboardCut.svg` → `ClipboardCut.svg` (nebo
   referenci), aby SVG cesta fungovala i pro Cut.
4. **Mimo rozsah**: ikony pluginů, doplnění chybějících SVG pro všechna
   tlačítka, varianta D (redesign) — případné samostatné featury.

Definitivní volba varianty (a zda P2 zařadit hned) je věcí `/speckit-plan`,
resp. upřesnění v `/speckit-clarify`.

## 5. Klíčové soubory

- `src\svg.cpp`, `src\svg.h` — SVG loader/rasterizace (nanosvg), ColorizeSVG
- `src\toolbar4.cpp` — tabulka tlačítek, `CreateToolbarBitmaps`,
  `RenderSVGImages`, grayscale/mask
- `src\salamdr1.cpp:2400-2538` — sestavení sdílených image listů
- `src\themes.cpp` / `src\themes.h` / `src\common\themes_palette.h` —
  theme engine, `ThemeAdjustBitmapForDarkMode`
- `src\res\toolbars\*.svg` + `src\vcxproj\!populate_build_dir.cmd:115` —
  assety a jejich nasazení do buildu
- `src\gui.cpp:2700-2838` — toolbar hlavičky edit-list boxů
- `src\plugins2.cpp:984` — ikony pluginů (mimo rozsah)
