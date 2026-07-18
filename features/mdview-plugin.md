# mdview – nový plugin pro prohlížení Markdown souborů

## 1. Cíl

Cílem úkolu je vytvořit nový plugin s názvem **mdview**, který bude sloužit k pohodlnému a vizuálně formátovanému prohlížení souborů ve formátu Markdown.

Po stisknutí klávesy **F3** nad souborem s příponou `.md` se soubor otevře v pluginu mdview. Uživatel neuvidí zdrojový Markdown text, ale jeho vykreslenou podobu včetně nadpisů, seznamů, tabulek, bloků kódu, odkazů a obrázků.

Plugin je určen pouze pro prohlížení. Úpravy Markdown souborů nejsou součástí tohoto úkolu.

---

## 2. Povinná předběžná analýza pomocí samostatných agentů

Před vytvořením výsledné specifikace alokuj několik samostatných analytických agentů. Každý agent musí problém posoudit z jiného pohledu.

Použij minimálně tyto role:

1. **Produktový a UX agent**

   * analyzuje očekávané chování uživatele,
   * navrhne ovládání prohlížeče,
   * identifikuje očekávání uživatelů známá z běžných Markdown prohlížečů,
   * posoudí přístupnost, čitelnost a ovládání pomocí klávesnice.

2. **Agent pro integraci pluginu**

   * analyzuje způsob napojení na F3 viewer,
   * prověří registraci pluginu pro příponu `.md`,
   * identifikuje možnosti a omezení stávajícího pluginového API,
   * navrhne chování při chybě nebo v případě, že dokument nelze vykreslit.

3. **Agent pro Markdown kompatibilitu**

   * navrhne podporovanou variantu Markdown syntaxe,
   * porovná CommonMark a běžně používaná rozšíření GitHub Flavored Markdown,
   * identifikuje požadované typy obsahu, odkazy, obrázky, tabulky a bloky kódu,
   * navrhne očekávané chování pro nepodporované konstrukce.

4. **Agent pro bezpečnost**

   * posoudí rizika raw HTML, JavaScriptu, vzdálených obrázků a externích odkazů,
   * navrhne bezpečný způsob vykreslování nedůvěryhodných Markdown souborů,
   * zajistí, že samotné zobrazení dokumentu nemůže automaticky spouštět aktivní obsah.

5. **Agent pro vizuální návrh a barevná schémata**

   * navrhne minimálně pět světlých a pět tmavých schémat,
   * posoudí kontrast, čitelnost a zvýraznění zdrojového kódu,
   * navrhne způsob výběru a trvalého uložení motivu.

6. **Agent pro testování a hraniční případy**

   * připraví testovací scénáře,
   * identifikuje problematické soubory, neplatný Markdown, chybějící obrázky, velké dokumenty a neobvyklá kódování,
   * navrhne měřitelná akceptační kritéria.

Jednotliví agenti musí pracovat nezávisle. Po dokončení jejich analýz výsledky porovnej, odstraň rozpory a vytvoř konsolidovanou specifikaci.

V této fázi nesmí být zahájena implementace.

---

## 3. Základní uživatelský scénář

1. Uživatel označí soubor s příponou `.md`.
2. Stiskne klávesu **F3**.
3. Systém předá soubor pluginu mdview.
4. Plugin načte obsah souboru.
5. Markdown se převede do bezpečné vykreslené podoby.
6. Dokument se zobrazí v samostatném vieweru.
7. Uživatel může:

   * dokument posouvat,
   * označovat a kopírovat text,
   * používat odkazy,
   * zobrazovat vložené obrázky,
   * přepnout barevné schéma,
   * viewer zavřít běžným způsobem, například klávesou `Esc`.

Při dalším otevření Markdown dokumentu se použije naposledy zvolené barevné schéma.

---

## 4. Podpora souborových formátů

Plugin **MUSÍ** být registrován minimálně pro:

* `.md`

Během analýzy ověř, zda má být plugin registrován také pro:

* `.markdown`
* `.mdown`
* `.mkd`

Rozšíření nad rámec `.md` nesmí být přidána bez zdůvodnění v návrhu specifikace.

---

## 5. Podporovaná Markdown syntaxe

Jako základ se předpokládá kompatibilita s **CommonMark**.

Plugin by měl podporovat také běžné prvky GitHub Flavored Markdown, protože mdview bude často používán pro zobrazování souborů `README.md` a další technické dokumentace.

Minimálně musí být analyzována podpora následujících prvků:

* nadpisy,
* odstavce a zalomení řádků,
* tučný text a kurzíva,
* přeškrtnutý text,
* číslované a nečíslované seznamy,
* vnořené seznamy,
* seznamy úkolů,
* citace,
* horizontální oddělovače,
* inline kód,
* bloky zdrojového kódu,
* označení programovacího jazyka u bloku kódu,
* syntax highlighting,
* odkazy,
* automaticky rozpoznané URL,
* obrázky,
* tabulky,
* escapované znaky,
* interní odkazy na nadpisy,
* HTML entity.

Agent pro Markdown kompatibilitu musí navrhnout přesný podporovaný standard a jeho případná rozšíření.

---

## 6. Obrázky

Plugin **MUSÍ** zobrazovat lokální obrázky odkazované relativně vůči adresáři otevřeného Markdown souboru.

Příklad:

```markdown
![Architektura](images/architecture.png)
```

Cesta `images/architecture.png` se musí vyhodnotit relativně vůči umístění otevřeného `.md` souboru.

Plugin musí korektně řešit:

* relativní cesty,
* cesty obsahující mezery,
* cesty obsahující Unicode znaky,
* odkazy na nadřazený adresář,
* neexistující soubor,
* nepodporovaný formát obrázku,
* poškozený obrázek,
* obrázek větší než dostupná zobrazovací plocha.

Velké obrázky se mají automaticky přizpůsobit šířce vieweru při zachování poměru stran. Nesmí dojít k nekontrolovanému rozšíření obsahu mimo zobrazovací plochu.

Minimálně musí být posouzena podpora formátů:

* PNG,
* JPEG,
* GIF,
* SVG,
* WebP,
* BMP.

Pokud obrázek nelze zobrazit, viewer musí zobrazit srozumitelný zástupný prvek obsahující alternativní text a informaci o chybě.

### Vzdálené obrázky

Automatické načítání vzdálených obrázků přes HTTP nebo HTTPS představuje bezpečnostní a soukromé riziko.

Během fáze `specify` musí být rozhodnuto, zda:

* budou vzdálené obrázky zcela zakázány,
* budou povoleny až po potvrzení uživatelem,
* bude existovat uživatelské nastavení,
* nebo budou povoleny automaticky pouze za přesně definovaných podmínek.

Výchozí návrh má preferovat bezpečné chování a nemá bez vědomí uživatele odesílat vzdálenému serveru informace o otevření dokumentu.

---

## 7. Odkazy

Viewer musí vizuálně rozlišovat odkazy od běžného textu.

Musí být analyzováno chování následujících typů odkazů:

* odkaz na nadpis v aktuálním dokumentu,
* odkaz na jiný lokální Markdown soubor,
* odkaz na jiný lokální soubor,
* HTTP a HTTPS odkaz,
* e-mailový odkaz,
* neplatný nebo nepodporovaný odkaz.

Požadované výchozí chování:

* interní odkaz na nadpis provede přesun v aktuálním dokumentu,
* externí odkaz se otevře pouze po explicitním kliknutí uživatele,
* otevření externího odkazu použije systémový výchozí prohlížeč,
* žádný odkaz se nesmí otevřít automaticky při načtení dokumentu,
* potenciálně nebezpečná schémata URL musí být blokována.

Navigace mezi několika lokálními Markdown soubory, včetně historie Zpět/Vpřed, musí být posouzena jako samostatná funkce. Pokud by výrazně rozšířila rozsah první verze, může být odložena do další etapy.

---

## 8. Bezpečnost vykreslování

Markdown soubor musí být považován za nedůvěryhodný vstup.

Plugin nesmí při otevření dokumentu:

* spouštět JavaScript,
* spouštět příkazy nebo aplikace,
* vykonávat HTML event handlery,
* automaticky odesílat formuláře,
* načítat aktivní objekty,
* vykonávat skripty vložené pomocí SVG,
* povolovat `iframe`, `object`, `embed` nebo obdobný aktivní obsah,
* automaticky otevírat externí URL.

Raw HTML musí být buď bezpečně sanitizováno, nebo zobrazeno jako neaktivní text. Zvolené řešení musí být zdokumentováno ve specifikaci.

---

## 9. Kódování textu

Plugin musí správně zobrazovat Unicode text včetně českých znaků.

Minimálním požadavkem je bezchybné načtení:

* UTF-8 bez BOM,
* UTF-8 s BOM.

Během analýzy ověř, zda lze využít společnou infrastrukturu aplikace pro detekci kódování a zda má plugin podporovat také například:

* UTF-16 LE/BE,
* Windows-1250,
* ISO-8859-2.

Pokud nelze kódování spolehlivě určit, nesmí dojít k pádu aplikace. Plugin musí použít definovaný fallback nebo umožnit otevření dokumentu v běžném textovém vieweru.

---

## 10. Barevná schémata

Viewer musí nabízet minimálně pět světlých a pět tmavých barevných schémat.

Každé schéma musí definovat minimálně:

* pozadí dokumentu,
* základní text,
* nadpisy,
* odkazy,
* navštívené nebo aktivní odkazy, pokud jsou podporovány,
* bloky citací,
* inline kód,
* bloky zdrojového kódu,
* tabulky a jejich ohraničení,
* horizontální oddělovače,
* zvýraznění označeného textu,
* syntax highlighting.

### Navržená světlá schémata

1. **Paper**

   * čisté bílé pozadí,
   * tmavě šedý text,
   * neutrální vzhled vhodný pro technickou dokumentaci.

2. **Soft Gray**

   * velmi světle šedé pozadí,
   * snížený kontrast mezi dokumentem a okolním prostředím,
   * vhodné pro dlouhé čtení.

3. **Warm Sepia**

   * teplé krémové pozadí,
   * tmavě hnědošedý text,
   * vzhled připomínající papír nebo elektronickou čtečku.

4. **Solar Light**

   * tlumené světle béžové pozadí,
   * barevně rozlišené nadpisy, odkazy a zdrojový kód,
   * inspirované motivy určenými pro dlouhodobou práci.

5. **Arctic Light**

   * studené světle modrošedé pozadí,
   * tmavě modrošedý text,
   * čistý technický vzhled.

### Navržená tmavá schémata

1. **Graphite**

   * tmavě šedé pozadí,
   * světle šedý text,
   * neutrální a málo rušivý vzhled.

2. **Midnight**

   * velmi tmavě modré pozadí,
   * světle modrošedý text,
   * vhodné pro večerní práci.

3. **Solar Dark**

   * tmavé modrozelené pozadí,
   * tlumené kontrastní barvy,
   * zaměřené na dlouhodobou čitelnost.

4. **Nordic Dark**

   * tmavé studené pozadí,
   * jemné modré a tyrkysové akcenty,
   * moderní technický vzhled.

5. **High Contrast Black**

   * téměř černé pozadí,
   * výrazně světlý text,
   * vysoký kontrast pro uživatele se zhoršeným zrakem.

Přesné barevné hodnoty musí být navrženy ve fázi návrhu. Barevná schémata musí splňovat přiměřené požadavky na kontrast a čitelnost.

---

## 11. Výběr a uložení barevného schématu

Uživatel musí mít možnost změnit schéma přímo ve vieweru.

Změna schématu:

* se projeví okamžitě,
* nesmí vyžadovat opětovné otevření souboru,
* nesmí změnit aktuální pozici v dokumentu,
* musí být dostupná pomocí myši i klávesnice.

Vybrané schéma se musí uložit do uživatelské konfigurace pluginu nebo aplikace.

Při příštím otevření Markdown souboru se automaticky použije poslední zvolené schéma.

Pokud uložené schéma již neexistuje nebo je konfigurace poškozena, plugin použije definované výchozí schéma a nesmí skončit chybou.

Během analýzy posuď také volitelný režim:

* **Podle motivu aplikace nebo operačního systému**.

Tento automatický režim může být doplňkem, ale nenahrazuje požadovaných pět světlých a pět tmavých schémat.

---

## 12. Ovládání vieweru

Viewer musí respektovat běžné ovládání používané v aplikaci.

Minimálně se očekává:

* `Esc` – zavření vieweru,
* šipky – posun obsahu,
* `Page Up` a `Page Down` – posun po stránkách,
* `Home` a `End` – začátek a konec dokumentu,
* kolečko myši – posun dokumentu,
* označení textu myší,
* `Ctrl+C` – zkopírování označeného textu,
* `Ctrl+A` – označení veškerého textu, pokud to použitá komponenta umožňuje.

Během analýzy musí být posouzena podpora:

* zvětšení a zmenšení textu,
* vyhledávání v dokumentu,
* kopírování odkazu,
* otevření zdrojové podoby dokumentu,
* obnovení dokumentu po změně souboru na disku.

Tyto funkce nejsou automaticky součástí minimální verze a musí být vyhodnoceny podle možností stávajícího vieweru.

---

## 13. Chybové stavy a fallback

Plugin nesmí způsobit pád aplikace ani v případě neplatného nebo poškozeného vstupu.

Musí být definováno chování alespoň pro tyto situace:

* soubor neexistuje,
* soubor nelze otevřít,
* uživatel nemá oprávnění ke čtení,
* neplatné nebo neznámé kódování,
* poškozený Markdown,
* chyba Markdown rendereru,
* chybějící obrázek,
* nepodporovaný obrázek,
* příliš velký dokument,
* nedostatek paměti,
* soubor se během prohlížení změní nebo odstraní.

Pokud vykreslení selže, uživatel musí dostat srozumitelnou informaci a možnost otevřít soubor v běžném textovém vieweru, pokud to pluginové API umožňuje.

---

## 14. Výkon a stabilita

Plugin musí zůstat použitelný i pro rozsáhlejší dokumenty.

Požaduje se:

* viewer nesmí při běžných dokumentech dlouhodobě blokovat hlavní UI aplikace,
* posouvání vykresleného dokumentu musí být plynulé,
* obrázky se musí načítat kontrolovaně,
* opakované otevření a zavření vieweru nesmí způsobovat úniky prostředků,
* dokument s neplatným Markdownem nesmí způsobit nekonečnou smyčku ani extrémní spotřebu paměti.

Agent pro testování musí navrhnout konkrétní testovací velikosti dokumentů a realistická měřitelná kritéria.

---

## 15. Přístupnost a zobrazení

Viewer musí:

* korektně respektovat DPI scaling systému,
* zachovat čitelnost při různých velikostech okna,
* umožnit kompletní základní ovládání klávesnicí,
* používat dostatečný kontrast textu a pozadí,
* nepoužívat pouze barvu jako jediný způsob předání významu,
* zobrazovat alternativní text obrázku, pokud obrázek nelze načíst.

---

## 16. Mimo rozsah první verze

Pokud analýza nedoporučí jinak, do první verze nepatří:

* editace Markdown souborů,
* živý náhled během editace,
* export do HTML nebo PDF,
* tisk,
* Mermaid diagramy,
* matematické výrazy pomocí MathJax nebo KaTeX,
* vykonávání JavaScriptu,
* interaktivní HTML formuláře,
* přehrávání vloženého videa nebo audia,
* pluginy třetích stran uvnitř Markdown dokumentu,
* automatické stahování vzdáleného aktivního obsahu.

Tyto funkce mohou být uvedeny jako možné budoucí rozšíření.

---

## 17. Akceptační kritéria

Úkol je možné považovat za splněný, pokud jsou splněny minimálně následující podmínky:

1. Stisknutí F3 nad `.md` souborem otevře mdview.
2. Dokument se zobrazí jako vykreslený Markdown, nikoliv pouze jako zdrojový text.
3. Správně se zobrazí české znaky v UTF-8.
4. Fungují nadpisy, odstavce, formátování textu, seznamy, citace, odkazy, tabulky a bloky kódu.
5. Bloky kódu jsou vizuálně odlišeny a podporují syntax highlighting alespoň pro běžně používané jazyky.
6. Lokální obrázek odkazovaný relativní cestou se správně zobrazí.
7. Velký obrázek se přizpůsobí šířce vieweru.
8. Chybějící obrázek nezpůsobí pád a zobrazí vhodný zástupný prvek.
9. Interní odkaz na nadpis přesune uživatele na správné místo dokumentu.
10. Externí odkazy se neotevírají automaticky.
11. Raw HTML nebo jiný aktivní obsah nemůže spustit JavaScript.
12. Viewer nabízí minimálně pět světlých a pět tmavých schémat.
13. Změna schématu se projeví okamžitě.
14. Zvolené schéma zůstane uložené po zavření aplikace.
15. Poškozená konfigurace motivu nezpůsobí pád.
16. Viewer lze ovládat klávesnicí a zavřít pomocí `Esc`.
17. Text lze označit a zkopírovat.
18. Neplatný nebo nestandardní Markdown nezpůsobí pád aplikace.
19. Při selhání vykreslování se zobrazí srozumitelná chyba nebo se nabídne fallback na textový viewer.
20. Opakované otevření a zavření různých Markdown souborů nezpůsobuje viditelnou degradaci nebo únik prostředků.

---

## 18. Požadovaný postup pomocí Spec Kit

Tento úkol bude implementován pomocí **Spec Kit flow**, ale jednotlivé fáze budeme spouštět ve spolupráci s uživatelem.

Nyní proveď pouze fázi:

```text
speckit specify
```

V rámci této fáze:

1. spusť požadované nezávislé analytické agenty,
2. sesbírej a porovnej jejich závěry,
3. identifikuj nejasnosti, rizika a chybějící požadavky,
4. vytvoř konsolidovanou specifikaci,
5. označ rozhodnutí, která bude potřeba potvrdit ve fázi `speckit clarify`.

Po dokončení fáze `specify` se zastav.

Nespouštěj automaticky:

* `speckit clarify`,
* `speckit plan`,
* `speckit tasks`,
* implementaci,
* změny zdrojového kódu.

Další fázi spustí uživatel samostatně. Nejde tedy o plně autonomní průchod celým flow, ale o postupnou spolupráci s uživatelem.
