# Úprava logiky pluginů

Na základě analýzy pluginů provedeme následující úpravy v projektu:

Pluginy, které budou zcela odstraněny:

- pak - Quake PAK archivy — čtení i zápis
- unarj - Rozbalování ARJ archivů
- unlha - Rozbalování LHA/LZH archivů
- unfat - Prohlížení obrazů FAT12/16/32 disket a disků
- wmobile - Přístup na Windows Mobile/CE zařízení přes RAPI/ActiveSync
- ieviewer - Prohlížeč HTML/XML/Markdown postavený na Internet Exploreru (MSHTML)
- splitcbn - Rozdělení souboru na části a zpětné spojení
- winscp - SFTP/SCP klient postavený na WinSCP 

Dále bude vytvořen konfigurační soubor v rootu projektu (navrhni vhodný formát konfiguračního souboru),
který bude načítán v rámci sestavení aplikace pomocí build.cmd a pomocí kterého bude definováno, jaké
pluginy se mají sestavit a být součástí buildu.

Následně vytvoř tento konfigurační soubor s následujícím nastavením:

Do konfiguračního souboru přidej všechny pluginy s tím, že následující pluginy budou v buildu vypnuty, tj.
nebudou se sestavovat a nebudou součástí buildu - což bude právě řídit tato konfigurace:

- unchm
- unmime
- unole
- mmviewer
- nethood
- automation 
- checkver
- demoplug
- demoview
- demomenu


