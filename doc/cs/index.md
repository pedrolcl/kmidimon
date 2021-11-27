% Index nápovědy

# Úvod

[Drumstick MIDI Monitor](https://kmidimon.sourceforge.io) zaznamenává nadcházející události 
z externího MIDI portu nebo aplikace
přes sekvencer ALSA nebo uloženy jako standardní MIDI soubory. to je
zvláště užitečné, pokud chcete ladit MIDI software nebo vaše MIDI nastavení.
Vyznačuje se pěkným grafickým uživatelským rozhraním, přizpůsobitelnými filtry událostí
a parametry sekvenceru, podpora všech MIDI zpráv a některých ALSA
zpráv a uložení seznamu zaznamenaných událostí do textového souboru nebo SMF.

# Začínáme

## Hlavní okno

Program se spustí ve stavu nahrávání, registruje všechny příchozí MIDI
události, dokud nestisknete tlačítko stop. K dispozici jsou také tlačítka pro přehrávání,
pozastavit, přetočit zpět a vpřed, s obvyklým chováním jakéhokoli jiného média
hráč.

Nad mřížkou seznamu událostí můžete najít sadu karet, pro každou jednu
stopa definovaná v SMF. Můžete přidat nové karty nebo zavřít karty bez
ztráty zaznamenaných událostí, protože se jedná pouze o pohledy nebo události
filtry.

Můžete ovládat MIDI připojení sekvenceru ALSA k programům a
zařízení zevnitř Drumstick MIDI monitoru. Chcete-li to provést, použijte možnosti v nabídce
"Připojení" v hlavní nabídce. Existují možnosti připojení a
odpojte každý dostupný vstupní port od Drumstick MIDI monitoru a také dialogové okno
kde si můžete vybrat porty, které mají být monitorovány, a výstupní.

Můžete také použít nástroj pro připojení MIDI, jako je
[aconnect(1)](https://linux.die.net/man/1/aconnect)
nebo [QJackCtl](https://qjackctl.sourceforge.io) pro připojení aplikace
nebo MIDI port pro Drumstick MIDI Monitor.

Když byl port MIDI OUT připojen ke vstupnímu portu Drumstick MIDI Monitor in
stav nahrávání, zobrazí příchozí MIDI události, pokud je vše v pořádku
opravit.

Každá přijatá MIDI událost je zobrazena na jednom řádku. Sloupce mají
následující význam.

* **Ticks**: Hudební čas příjezdu události
* **Čas**: Skutečný čas v sekundách od příchodu události
* **Zdroj**: ALSA identifikátor MIDI zařízení, ze kterého pochází
    událost. Budete schopni rozpoznat, jaká událost ke které patří
    zařízení, pokud je připojeno několik současně. Tady je
    konfigurační volba pro zobrazení názvu klienta ALSA namísto a
    číslo
* **Druh události**: Typ události: nota on/off, control change, ALSA a
    již brzy
* **Chan** MIDI kanál události (pokud se jedná o událost kanálu). To
    se také používá k zobrazení kanálu Sysex.
* **Data 1**: Záleží na typu přijímané události. Pro kontrolu
    Změnit událost nebo poznámku, je to kontrolní číslo nebo číslo poznámky
* **Data 2**: Záleží na typu přijímané události. Pro kontrolu
    Změnit to bude hodnota a pro událost Note to bude
    rychlost
* **Data 3**: Textová reprezentace System Exclusive nebo Meta událostí.

Pomocí kontextové nabídky můžete libovolný sloupec skrýt nebo zobrazit. Chcete-li to otevřít
stiskněte sekundární tlačítko myši nad seznamem událostí. Můžete také
pomocí dialogového okna Konfigurace vyberte viditelné sloupce.

Seznam událostí vždy zobrazuje novější zaznamenané události ve spodní části
mřížka.

Drumstick MIDI Monitor umí uložit zaznamenané události jako textový soubor (ve formátu CSV) popř
standardní MIDI soubor (SMF).

## Možnosti konfigurace 

Dialog Konfigurace otevřete v menu Nastavení → Konfigurovat
nebo klikněte na příslušnou ikonu na panelu nástrojů.

Toto je seznam možností konfigurace.

* **Karta Sekvencer**. Výchozí nastavení fronty má vliv na čas události
    přesnost.
* **Karta Filtry**. Zde můžete zkontrolovat několik rodin událostí
    zobrazeny v seznamu událostí.
* **Karta Zobrazení**. První skupina zaškrtávacích políček umožňuje zobrazit/skrýt
    odpovídající sloupce seznamu událostí.
** Různé záložka**. Mezi různé možnosti patří:
    + Přeložte ID klientů ALSA na jména. Pokud je zaškrtnuto, klient ALSA
      jména se používají místo ID čísel ve sloupci "Zdroj" pro
      všechny krále událostí a datové sloupce pro události ALSA.
    + Překladač zpráv Universal System Exclusive. Pokud je zaškrtnuto,
      Universal System Exclusive zprávy (v reálném čase, mimo reálný čas,
      MMC, MTC a několik dalších typů) jsou interpretovány a překládány.
      Jinak se zobrazí hexadecimální výpis.
    + Použít pevné písmo. Ve výchozím nastavení Drumstick MIDI Monitor používá systémové písmo v
      seznam událostí. Pokud je tato možnost zaškrtnuta, použije se pevné písmo
      namísto.

# Kredity a licence

Copyright © 2005-2021 Pedro Lopez-Cabanillas

Dokumentace Copyright © 2005 Christoph Eckert

Dokumentace Copyright © 2008-2021 Pedro Lopez-Cabanillas

# Instalace

## Jak získat Drumstick MIDI Monitor 

[Zde](https://sourceforge.net/projects/kmidimon/files/)
můžete najít poslední verzi. K dispozici je také zrcadlo Git
[GitHub](https://github.com/pedrolcl/kmidimon)

## Požadavky

Abyste mohli úspěšně používat Drumstick MIDI Monitor, potřebujete Qt 5, Drumstick 2
a ALSA ovladače a knihovna.

Systém sestavení vyžaduje [CMake](http://www.cmake.org) 3.14 nebo novější.

Knihovnu ALSA, ovladače a nástroje naleznete na
[Domovská stránka ALSA](http://www.alsa-project.org)

Seznam změn najdete na https://sourceforge.net/p/kmidimon

## Kompilace a instalace

Chcete-li zkompilovat a nainstalovat Drumstick MIDI Monitor na váš systém, zadejte
následující v základním adresáři distribuce Drumstick MIDI Monitor:

    % cmake .
    % udělat
    % provést instalaci

Protože Drumstick MIDI Monitor používá `cmake` a `make`, neměli byste mít žádné potíže
jeho sestavování. Pokud narazíte na problémy, nahlaste je na
autora nebo systému sledování chyb projektu.

