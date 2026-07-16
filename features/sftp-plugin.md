# SFTP plugin — zadání

## Kontext a cíl
Navrhni a implementuj nový plugin **sftp**, který umožní připojení a plnohodnotnou
práci se soubory na vzdáleném serveru přes **SFTP (nad SSH)**. Cíl: co nejlepší
použitelnost při správě **linuxových serverů**.

## 0. Analýza (výstup PŘED implementací)
Prostuduj stávající **FTP plugin** a předlož mi krátký design dokument, který odpoví na:
- Jak vypadá kontrakt pluginu (které třídy/metody se implementují, jak se plugin registruje).
- Jak FTP plugin řeší: dialog připojení, správu a ukládání připojení, ukládání hesla (kde a jak bezpečně).
- Jak hostitelská aplikace vykresluje sloupce a atributy souborů (odkud se berou „windows atributy").
- Jaký jazyk/framework a jaké závislosti jsou v projektu k dispozici.
Analýzu mi dej potvrdit, teprve pak implementuj.

## 1. UI a integrace (MUST)
- Zachovej **velmi podobné UI a integraci jako FTP plugin** — dialog připojení, okna,
  nastavení, správa a ukládání připojení včetně volby uložit heslo.
- Kde to jde, **znovupoužij** společný základ FTP pluginu, neduplikuj kód.
- Pole připojení: host, **port (default 22)**, uživatel, metoda autentizace, (volitelně) výchozí vzdálená cesta.

## 2. Autentizace (MUST)
- **Heslo** — stejné bezpečné úložiště jako FTP plugin.
- **Privátní SSH klíč** per připojení: výběr souboru, běžné formáty (OpenSSH, PEM/PKCS#8, ideálně i PuTTY .ppk),
  podpora **passphrase** (volitelně uložit jako heslo).
- (NICE) ssh-agent / Pageant, keyboard-interactive.
- **Ověření host key (MUST — bezpečnost):** při prvním připojení zobraz fingerprint a nabídni uložení
  (known_hosts / TOFU), při změně klíče varuj. Žádné tiché přijetí.

## 3. Souborové operace (MUST)
List adresáře, download, upload (ideálně s resume), rename/move, delete, mkdir, rmdir,
symlinky (číst cíl i vytvořit), **chmod**, nastavení času; (NICE) chown/chgrp.
Korektní práce s **UTF-8 názvy** souborů.

## 4. Zobrazení práv — KLÍČOVÉ (MUST)
- Zobrazuj **klasická linuxová práva** místo windows atributů: symbolicky `drwxrwxrwx`
  a (volitelně) oktalově `0755`.
- Rozliš typ záznamu (`-`, `d`, `l`, …) a **speciální bity** (setuid/setgid/sticky).
- Zobraz **vlastníka a skupinu** (u SFTP v3 čísla uid/gid, u v4+ jména).
- Umožni **změnu práv (chmod dialog)** přímo z pluginu.
- Ideálně přepínatelné zobrazení (unix práva vs. původní atributy) tak, ať sedí do stávajících sloupců.

## 5. Nefunkční požadavky
Timeouty a keepalive, korektní **reconnect** po výpadku, srozumitelné chybové hlášky,
zvládnutí velkých souborů a velkých adresářů; (NICE) paralelní přenosy, komprese.

## 6. Knihovna
Vyber SSH/SFTP knihovnu podle jazyka projektu (např. .NET: SSH.NET; Java: sshj/JSch;
C/C++: libssh2) a zdůvodni volbu v analýze. Přednostně existující závislost projektu.

## 7. Definition of Done
- Připojení heslem i klíčem funguje proti reálnému OpenSSH serveru.
- Uložená připojení (vč. hesla/klíče) přežijí restart — stejně jako u FTP.
- Práva se zobrazují jako `drwxrwxrwx` a jdou změnit.
- Základní operace ověřené testy; **nerozbít stávající FTP plugin**.
- Dodržen styl kódu projektu; pokud je aplikace lokalizovaná, přeložené texty.