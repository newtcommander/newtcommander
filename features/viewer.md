# Viewer F3 – správná detekce a zobrazení kódování textových souborů

## Popis problému

Při otevření textového souboru pomocí funkce **F3 – Viewer** se některé soubory zobrazují s nesprávným kódováním znaků. Problém je patrný zejména u českých znaků s diakritikou.

Například soubor uložený v kódování UTF-8 je ve Vieweru chybně interpretován jako ISO-8859-2 nebo Windows-1250. V titulku okna se následně zobrazuje nesprávně určené kódování a text obsahuje poškozené nebo nesprávně zobrazené znaky.

## Cíl úpravy

Přepracovat načítání textových souborů ve Vieweru tak, aby:

* správně detekoval běžná textová kódování,
* text převáděl do interního Unicode formátu,
* správně zobrazoval znaky včetně české diakritiky,
* zobrazoval skutečně použité kódování,
* umožňoval uživateli kódování ručně změnit, pokud automatická detekce není jednoznačná.

## Požadované chování

### Automatická detekce kódování

Při otevření textového souboru MUSÍ Viewer určit kódování v následujícím pořadí:

1. Pokud soubor obsahuje BOM, MUSÍ být kódování určeno podle BOM.
2. Pokud soubor neobsahuje BOM, MUSÍ být ověřeno, zda celý obsah odpovídá validnímu UTF-8.
3. Pokud je obsah validní UTF-8, MUSÍ být soubor zobrazen jako UTF-8.
4. Pokud obsah není validní UTF-8, MŮŽE Viewer použít detekci některého z podporovaných legacy kódování.
5. Pokud nelze kódování spolehlivě určit, MUSÍ být použito definované výchozí kódování a uživatel MUSÍ mít možnost zvolit jiné kódování ručně.

Minimálně MUSÍ být podporována následující kódování:

* UTF-8,
* UTF-8 s BOM,
* UTF-16 LE,
* UTF-16 BE,
* Windows-1250,
* ISO-8859-2.

Pokud Viewer již podporuje další kódování, jejich podpora NESMÍ být touto změnou odstraněna.

### Zobrazení textu

Viewer MUSÍ před vykreslením převést načtený text do interního Unicode formátu používaného aplikací.

Viewer NESMÍ vykreslovat původní bajty souboru přímo pomocí systémové ANSI znakové sady.

Správně se MUSÍ zobrazit zejména:

* české znaky s diakritikou,
* znaky dalších evropských jazyků,
* vícebajtové Unicode znaky,
* soubory obsahující kombinaci ASCII a Unicode znaků.

### Informace o kódování

Viewer MUSÍ zobrazovat kódování, které bylo skutečně použito při dekódování souboru.

Informace o kódování MŮŽE být zobrazena:

* v titulku okna,
* ve stavovém řádku,
* případně na obou místech podle současného návrhu aplikace.

Viewer NESMÍ zobrazovat kódování, které bylo pouze předběžně odhadnuto, pokud byl soubor následně dekódován pomocí jiného kódování.

### Ruční změna kódování

Uživatel MUSÍ mít možnost změnit kódování otevřeného souboru bez jeho zavření.

Po změně kódování MUSÍ Viewer:

1. znovu načíst nebo znovu dekódovat původní obsah souboru,
2. aktualizovat zobrazený text,
3. aktualizovat informaci o použitém kódování.

Ruční změna kódování NESMÍ upravit obsah souboru na disku.

## Binární soubory

Změna NESMÍ způsobit, že budou zjevně binární soubory automaticky interpretovány jako běžný text.

Současné rozpoznávání textového a binárního režimu MUSÍ zůstat funkční, případně musí být upraveno tak, aby správně spolupracovalo s novou detekcí kódování.

## Výkon

Detekce kódování NESMÍ způsobit výrazné zpomalení otevření velkých souborů.

U velkých souborů MŮŽE být pro prvotní detekci analyzována pouze omezená část souboru. Validace UTF-8 však MUSÍ být navržena tak, aby pozdější části souboru nemohly způsobit chybné nebo nekonzistentní zobrazení.

Viewer NESMÍ kvůli detekci kódování vytvářet zbytečnou kopii celého souboru v paměti, pokud současná implementace podporuje postupné nebo mapované načítání.

## Zpětná kompatibilita

Úprava NESMÍ narušit:

* zobrazování ASCII souborů,
* zobrazování binárních souborů,
* vyhledávání ve Vieweru,
* posouvání a navigaci v souboru,
* zobrazování velmi dlouhých řádků,
* otevírání velkých souborů,
* existující klávesové zkratky Vieweru.

## Akceptační kritéria

Úprava je považována za dokončenou, pokud jsou splněny následující podmínky:

1. UTF-8 soubor bez BOM obsahující českou diakritiku se zobrazí správně a je označen jako UTF-8.
2. UTF-8 soubor s BOM se zobrazí správně a je rozpoznán podle BOM.
3. UTF-16 LE a UTF-16 BE soubory se zobrazí správně.
4. Soubor ve Windows-1250 obsahující českou diakritiku lze zobrazit správně.
5. Soubor v ISO-8859-2 obsahující českou diakritiku lze zobrazit správně.
6. Uživatel může ručně přepnout kódování a text se okamžitě znovu zobrazí.
7. Ruční přepnutí kódování nezmění soubor na disku.
8. Informace o kódování odpovídá kódování skutečně použitému pro zobrazení.
9. ASCII soubory se zobrazují stejně jako před úpravou.
10. Binární soubory se nezačnou automaticky zobrazovat jako text.
11. Nedojde k významnému zhoršení rychlosti otevírání velkých souborů.

## Testovací data

Součástí implementace BY MĚLY být testovací soubory se stejným referenčním textem uloženým minimálně v těchto variantách:

* UTF-8 bez BOM,
* UTF-8 s BOM,
* UTF-16 LE,
* UTF-16 BE,
* Windows-1250,
* ISO-8859-2,
* čisté ASCII,
* nevalidní UTF-8,
* binární soubor obsahující náhodné sekvence bajtů.

Referenční text MUSÍ obsahovat české znaky:

`Příliš žluťoučký kůň úpěl ďábelské ódy.`

Testovací text BY MĚL obsahovat také znaky dalších jazyků a několik znaků mimo základní BMP Unicode rozsah.
