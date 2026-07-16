# Hodnocení pluginů — údržba pro rok 2026+

> **Aktualizace (2026-07-16, feature 007 — plugin build policy):**
> doporučení z tohoto dokumentu byla realizována. Osm pluginů bylo
> z repozitáře zcela odstraněno (pak, unarj, unlha, unfat, wmobile,
> ieviewer, splitcbn, winscp) a deset dalších je ve výchozím buildu
> vypnuto přes `plugins.cfg` v kořeni repozitáře (automation, checkver,
> demomenu, demoplug, demoview, mmviewer, nethood, unchm, unmime,
> unole). Výchozí build tedy obsahuje 18 pluginů. Podrobnosti viz
> `specs/007-plugin-build-policy/`.

Kompletní přehled všech 36 pluginů po kategoriích, s popisem a hodnocením
smysluplnosti údržby pro rok 2026+. Doplňuje katalog
[09-plugin-catalog.md](09-plugin-catalog.md).

Legenda verdiktu: ✅ udržovat, ⚠️ udržovat, ale vyžaduje modernizaci,
❌ kandidát na vyřazení (údržba má jen historickou/retro hodnotu).

## Archivátory — plná podpora (čtení i zápis)

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **zip** | Vytváření, prohlížení a rozbalování ZIP archivů (`zip;pk3;pk4;jar`), včetně SFX | ✅ Naprostý základ — ZIP je stále všudypřítomný standard. Priorita č. 1. |
| **7zip** | Vytváření, prohlížení a rozbalování 7z archivů (přes přibalený 7za engine) | ✅ 7z je dnes de facto standard komprese na Windows. Udržovat, průběžně aktualizovat 7-Zip engine. |
| **pak** | Quake PAK archivy — čtení i zápis | ❌ Formát herního enginu z roku 1996. Dnes zajímavé jen pro retro/modding komunitu. Údržba levná, ale přínos minimální. |

## Archivátory — jen rozbalení

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **tar** | Unixové archivy: `tar, tgz, gz, bz2, rpm, cpio, deb`... | ✅ Díky WSL, kontejnerům a Linux interoperabilitě relevantnější než kdy dřív. Stálo by za to doplnit `xz`/`zst`. |
| **unrar** | Rozbalování RAR archivů | ✅ RAR je stále velmi rozšířený. Háček: vyžaduje `unrar.dll`, která není v repozitáři — vyřešit distribuci závislosti. |
| **uncab** | Rozbalování CAB archivů | ✅ CAB dodnes používá samotný Windows (ovladače, aktualizace, MSU/MSIX). Užitečný diagnostický nástroj. |
| **unchm** | Prohlížení a rozbalování CHM (Compiled HTML Help) | ⚠️ Mrtvý formát nápovědy, ale stále se s ním lze potkat (starší dokumentace, e-booky). Udržovat pasivně, neinvestovat. |
| **unmime** | Dekódování MIME/Base64, UU/XXEncode, yEncode, BinHex (`eml`...) | ⚠️ Rozbalování příloh z `.eml` má pořád smysl; UUencode/BinHex jsou mrtvé. Udržovat kvůli EML, zbytek je balast. |
| **unole** | Rozbalování OLE Compound souborů (staré `.doc`/`.xls` kontejnery) | ⚠️ Okrajová diagnostická hodnota (starý Office, MSI). Pasivní údržba. |
| **unarj** | Rozbalování ARJ archivů | ❌ Formát z DOS éry 90. let. Jen archeologie. |
| **unlha** | Rozbalování LHA/LZH archivů | ❌ Mrtvý formát (přežívá okrajově v Japonsku a na Amize). Jen archeologie. |

## Diskové obrazy

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **uniso** | Prohlížení/rozbalení CD/DVD obrazů (`iso;nrg;img;dmg`...) | ✅ ISO je stále běžné (instalační média, distribuce SW). Windows sice umí ISO připojit nativně, ale prohlížení v panelu bez mountování je přidaná hodnota — a podpora `nrg`/`dmg` atd. je unikátní. |
| **unfat** | Prohlížení obrazů FAT12/16/32 disket a disků (`ima`) | ❌ Disketové obrazy = retro niche (embedded vývojáři výjimečně). Kandidát na vyřazení nebo čistě pasivní režim. |

## Prohlížeče souborů

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **pictview** | Rychlý prohlížeč obrázků, náhledy, EXIF | ✅ Klíčová funkce file manageru. Po přechodu na WIC engine (feature 006) je zbaven proprietární závislosti a připraven na budoucnost (WIC dává zdarma HEIC/WebP/AVIF přes systémové kodeky). |
| **ieviewer** | Prohlížeč HTML/XML/Markdown postavený na Internet Exploreru (MSHTML) | ⚠️ Účel (rychlý náhled HTML/MD) je stále platný, ale IE/Trident je mrtvý — MSHTML je ve Windows už jen legacy komponenta. Nutná migrace na **WebView2**, jinak časem přestane fungovat. |
| **mmviewer** | Zobrazení metadat multimediálních souborů (MP3, OGG, WAV...) | ⚠️ Užitečné, ale úzké; chybí moderní formáty (M4A/FLAC/Opus). Udržovat levně, případně rozšířit. |
| **peviewer** | Informace o PE souborech (EXE/DLL — hlavičky, sekce, importy) | ✅ Malý, samostatný, pro vývojáře a power-usery stále cenný. Levná údržba. |
| **dbviewer** | Prohlížení dBase/FoxPro/CSV databází | ⚠️ CSV část je stále velmi relevantní; DBF je legacy. Stálo by za modernizaci (SQLite, případně XLSX náhled), jádro udržovat. |

## Souborové nástroje

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **filecomp** | Vizuální porovnání dvou textových/binárních souborů | ✅ Klasická výbava file manageru, stále užitečná. |
| **renamer** | Hromadné přejmenování s náhledem, vzory a regexy | ✅ Jedna z nejžádanějších funkcí dvoupanelových manažerů. Určitě udržovat. |
| **checksum** | Výpočet/ověření SFV, MD5, SHA-1/256/512 | ✅ Ověřování integrity je běžná potřeba. Zvážit doplnění SHA-3/BLAKE3 a využití moderních CPU instrukcí. |
| **diskmap** | Treemapa obsazení disku (à la WinDirStat) | ✅ „Co mi žere disk" je věčná otázka. Udržovat. |
| **splitcbn** | Rozdělení souboru na části a zpětné spojení | ⚠️ Relikt éry disket a FAT limitu 4 GB; občas se hodí (FAT32 USB, limity uploadů). Pasivní údržba. |

## Síť / vzdálený přístup

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **ftp** | FTP klient jako virtuální FS v panelu (`ftp:`) | ⚠️ FTP upadá, ale žije (embedded zařízení, NAS, legacy servery). FTPS dnes závisí na chybějícím OpenSSL — modernizovat na systémový Schannel. Skutečná mezera je ale SFTP (viz winscp). |
| **winscp** | SFTP/SCP klient postavený na WinSCP (`winscp:`) | ❌/⚠️ Účel je vysoce relevantní (SFTP je dnešní standard), ale implementace je mrtvá: bez `.vcxproj`, jen x86, vyžaduje Embarcadero RTL. Nemá smysl udržovat v této podobě — buď přepsat (např. nad libssh2), nebo vyřadit. SFTP je největší funkční díra kategorie. |
| **nethood** | Procházení počítačů v síti / SMB sdílení (`net:`) | ⚠️ SMB sdílení v LAN je stále běžné, ale discovery se změnilo (NetBIOS je mrtvý, dnes WSD/mDNS). Udržovat a modernizovat vyhledávání. |

## Přenosná zařízení

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **portables** | Přístup na MTP/PTP zařízení přes WPD API (`pd:`) — telefony, fotoaparáty | ✅ Android telefony komunikují výhradně přes MTP — vysoká hodnota pro běžné uživatele. Udržovat a spíš investovat. |
| **wmobile** | Přístup na Windows Mobile/CE zařízení přes RAPI/ActiveSync (`CE:`) | ❌ Platforma mrtvá přes deset let (podpora skončila ~2013). Jasný kandidát na vyřazení. |

## Systém a integrace

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **regedt** | Windows registry jako souborový systém v panelu (`reg:`), s vyhledáváním | ✅ Unikátní power-user nástroj, registry nikam nemizí. |
| **undelete** | Obnova smazaných souborů z FAT/NTFS (`del:`) | ⚠️ Na HDD, USB flash a SD kartách stále funguje a pomáhá; na SSD ji TRIM prakticky eliminuje. Udržovat, ale nečekat zázraky na moderních discích. |
| **folders** | Procházení shell namespace — Plocha, Ovládací panely, Koš (`fld:`) | ✅ Doplňuje integraci s Windows shellem, malá údržba. |
| **automation** | Automatizace Salamanderu skripty (COM / Active Scripting) | ⚠️ Účel důležitý, ale postavený na VBScript/JScript — VBScript Microsoft aktivně odstraňuje z Windows. Nutná náhrada enginu (PowerShell, moderní JS, Lua...). |
| **checkver** | Kontrola aktualizací Salamanderu a pluginů online | ⚠️ Ukazuje na servery původního Altapu — pro open-source éru je potřeba přesměrovat na GitHub Releases API, jinak je k ničemu. |

## Ukázkové pluginy (SDK)

| Plugin | K čemu je | Verdikt 2026+ |
|--------|-----------|---------------|
| **demoplug** | Kompletní ukázka: archivátor + viewer + FS + menu | ✅ Udržovat — slouží jako živá dokumentace SDK a kompilační test celého plugin API (zvlášť po ABI změně v interface 104). |
| **demoview** | Minimální ukázka viewer pluginu | ✅ Totéž. |
| **demomenu** | Minimální ukázka menu rozšíření | ✅ Totéž. |

## Shrnutí

- **Jádro k aktivní údržbě (18):** zip, 7zip, tar, unrar, uncab, uniso, pictview, peviewer, filecomp, renamer, checksum, diskmap, portables, regedt, folders + 3 dema.
- **Udržovat s nutnou modernizací (5):** ieviewer (MSHTML → WebView2), automation (náhrada VBScript), checkver (GitHub Releases), ftp (Schannel místo OpenSSL), nethood (moderní discovery).
- **Pasivní/legacy režim (6):** unchm, unmime, unole, splitcbn, dbviewer (DBF část), undelete (SSD limitace).
- **Kandidáti na vyřazení (6):** wmobile (mrtvá platforma), unarj, unlha, unfat, pak (retro formáty) a winscp v současné podobě — s tím, že **SFTP by si zasloužilo novou implementaci**, protože je to dnes nejcitelnější chybějící funkce.
