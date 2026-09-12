# Philips Hue OmniGlow — Nederlandse handleiding

Dit is een onafhankelijke test- en onderzoekshandleiding voor één geteste Philips Hue OmniGlow-lichtstrip. Het is geen officiële Philips-documentatie. De gebruikte chip, de precieze kanaalvolgorde, de elektrische grenswaarden en de betekenis van de laatste zes bytes zijn nog niet vastgesteld.

## Eerst de bedrading veilig maken

1. Schakel de strip uit.
2. Isoleer de DATA-draad van de originele Hue-controller fysiek. Die controller mag DATA niet tegelijk met de Arduino aansturen.
3. Gebruik de geverifieerde DATA- en GND-aansluiting en houd de gemeenschappelijke massa aan zoals in de testopstelling.
4. De strip gebruikt in deze opstelling een nominale voeding van 24 V. Zet nooit 24 V op een Arduino-signaalpin.
5. Sluit de geverifieerde DATA van de strip aan op D2. Neem draadkleuren niet over naar een andere strip.

De Arduino staat bij het opstarten en na stoppen op `INPUT` en start geen lichtsignaal uit zichzelf.

## Hardware en timing

De software is gebouwd voor een Arduino Uno met ATmega328P op 16 MHz. De seriële verbinding gebruikt 115200 baud.

De werkende, door de Arduino gegenereerde golf heeft een symboolduur van 20 CPU-cycli, oftewel 1,25 µs. Een korte hoge puls duurt 4 cycli (250 ns), een lange hoge puls 15 cycli (937,5 ns). Een werkend frame bevat 806 verzonden bytes, samen 6.448 symbolen, en duurt ongeveer 8,06 ms.

De huidige werkweergave behandelt de eerste 800 bytes als 80 herhaalde eenheden van 10 bytes en bewaart daarna de vaste staart `03 75 D7 5E 78 08`. Dat is een bruikbaar onderzoeksmodel, geen bewezen Philips-veldindeling. De 80 eenheden zijn niet bewezen 80 leds. De volgorde van RGB-, wit- of CCT-kanalen is evenmin definitief bewezen.

## Seriële commando’s

| Commando | Werking |
|---|---|
| `D` | Start 12 overgangen met vloeiende willekeurige kleuren. Beide helften krijgen afzonderlijk een doelkleur. |
| `W` | Start de test voor witte kanalen. |
| `P` | Start de playground met acht effecten. |
| `X` | Stop tussen twee frames en zet D2 terug op `INPUT`. |

Stuur één hoofdletter per keer via de seriële monitor. Tijdens een effect wordt `X` tussen frames gecontroleerd. Na een normale afronding of na `X` wordt D2 eerst laag gezet en daarna vrijgegeven als `INPUT`.

## Wat het bewijs wel zegt

Op de geteste intacte strip zijn volledig rood, volledig blauw, een verdeling van 40 eenheden rood en 40 blauw, en de omgekeerde verdeling door de gebruiker bevestigd. Dat bewijst dat deze werkende golfvorm op deze strip reageert.

De kleurwaarden zijn afgeleid uit metingen met een gekozen korte/lange-pulsnotatie. De eerste puls van het oorspronkelijke signaal was niet volledig vastgelegd; de eerste positie is daarom afgeleid uit de herhaling. De vaste staart lijkt stabiel, maar er is geen bewijs dat die een CRC of checksum is.

Gebruik deze resultaten dus als een strip-specifieke testbasis. Noem het geen algemene Philips Hue- of WS2812-protocolimplementatie en verbind de 80 eenheden niet automatisch aan een fysiek led-aantal.

## Document en volledige software

Het [complete document](COMPLETE_PROTOCOL_AND_SOFTWARE.md) bevat de protocolspecificatie en de volledige huidige Arduino-code. De losse, uploadbare code staat in [omniglow_playground.ino](../firmware/omniglow_playground/omniglow_playground.ino). Bouw- en uploadinstructies staan in de [README](../README.md#build-validate-and-run).

`P` speelt acht ruimtelijke effecten: noorderlicht, dansende kometen, kringen, stuiterende lichtbolletjes, snoepkleurige banen, vuurvliegjes, zonsopkomst en vloeiende kleurpatronen. `W` test wit/warm en enkele effecten. `D` laat beide helften afzonderlijk van kleur veranderen. De demonstraties zijn begrensd en starten alleen op commando.

## Volgende stap: twee lampen in Hue

Eerst één klein Zigbee-bordje met aangepaste software daadwerkelijk aan een Hue Bridge koppelen. Test aan/uit, dimmen, kleuren, kleurtemperatuur en herstel na stroomuitval. Pas daarna een tweede exemplaar en een fysiek gesplitste strip proberen. Deze repository bevat nog geen werkende Zigbee-koppeling en bewijst niet dat een afgeknipt deel zelfstandig werkt.
