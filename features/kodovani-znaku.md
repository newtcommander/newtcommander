# Kompletní revize zobrazení názvů souborů a kódování znaků

Proveď detailní hloubkovou analýzu celého programu a identifikuj **VŠECHNA** místa, kde se zobrazují názvy souborů,
resp. cest. V jednom z předchozích úkolů jsme předělávali kódování Unicode a podporu pro dlouhé názvy souborů.
Stále se ale v programu nacházejí místa, kde je zobrazení znaků cest, resp. souborů chybné.

Jedním z takových míst je např. Directory Line - viz zobrazení: G:\Můj disk\AI, zobrazí se jako: G:\MĹŻj disk

Dej si pozor ať se podíváš na všechna místa výskutu a použítí názvu souorů a cest včetně pluginů.

Zaměř se i na ostatní výskytu kódování - např. když dám ALT+F5 tak tak se zobrazí popup okno pack a packer
je select, ve kterém jsou položky se španým kódováním znaků.

**proveř vše**
