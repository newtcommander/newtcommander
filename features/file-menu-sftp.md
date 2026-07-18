# File menu pro soubory v SFTP pluginu

Toto rozšíření přidává podporu souborového menu při kliknutí pravým tlačítkem myši na 
soubor nebo adresář v SFTP připojení.

V současné verzi se nic neděje. Cílem rozšíření je přidat file-menu s položkami, které
jsou analogické pro soubory zobrazené přes FTP plugin. Především pak možnost nastavení 
atributů souboru drwxrwxrwx. To je kruciální.

Alokuj alespoň dva samostatné agenty, kteří prověří a analyzují jak funguje toto
menu a nastavení atributů a případně další funkce v pluginu FTP. Následně
implementuj stejně i pro SFTP plugin, resp. jako rozšíření práci se soubory a
adresáři v SFTP pluginu.

Dále přidej možnost změny vlastníka, resp. skupiny souboru a to včetně možnosti
změny na úrovni adresáře. V případě změny u adresáře zároveň přidej volbu,
která umožní změnu provést rekurzivně pro všechny vnořené podadresáře a soubory.

Implementuj toto rozšíření pomocí celého flow speckit zcela samostatně.
Tento popis slouží jako vstupní instrukce pro /speckit-specify. Následně
zcela autonomě proveď celé flow, tedy po /speckit-specify proveď /speckit-plan,
/speckit-tasks a následně /speckit-implement.

Každý krok a implementaci prověřuj a kontroluj.
