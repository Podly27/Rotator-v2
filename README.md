# Arduino Rotátor

Rotátor pro anténní systémy s víceotáčkovým potenciometrem, TFT displejem a bezpečnostní logikou relé.

![Rotátor](rotator-ovladac.jpg)

## Verze 2.0 – dvouřídicí architektura UNO + NANO

Nová verze řeší rušení na dlouhém analogovém vedení tím, že:
- **NANO u rotátoru** čte potenciometr lokálně (A0), filtruje hodnotu a posílá ji digitálně.
- **UNO v shacku** přijímá data po 1 vodiči (open-collector UART), řídí TFT, relé, limity a piezo.

Komunikace: `P,<adc>,<crc>\n` při 9600 Bd.

## Quick start (v2)

1. Nahraj `rotator_node_nano/rotator_node_nano.ino` do **Arduino NANO**.
2. Nahraj `rotator_main/rotator_main.ino` do **Arduino UNO**.
3. Propoj 3 žíly mezi UNO a NANO:
   - GND
   - +V (5–12 V, při >5 V přes step-down na 5 V pro NANO)
   - DATA (open-collector dle `zapojeni_schema.txt`)
4. Ověř, že UNO přijímá data (na TFT stav `OK`, při výpadku `ERROR`).

Poznámky k pinům (v2):
- TFT ST7789: CS=10, DC=8, RST=9, MOSI=11, SCK=13
- 1-wire UART (UNO): RX=D4, TX=D5 (vyhýbá se TFT pinům 8/9/10/11/13)

## 📁 Struktura projektu

```
Rotator/
├── rotator_main/
│   └── rotator_main.ino            # UNO (shack) – hlavní program (verze 2)
├── rotator_node_nano/
│   └── rotator_node_nano.ino       # NANO (rotátor) – čtení potenciometru + TX
├── legacy/
│   ├── rotator_main_analog_v1.ino  # původní analogová verze (archiv)
│   └── zapojeni_schema_v1.txt      # původní schéma analog A0 (archiv)
├── zapojeni_schema.txt             # aktuální schéma (verze 2)
├── PROJEKT.md
└── README.md
```

## Legacy / v1

Původní analogová varianta (A0 po dlouhém kabelu + MCP6001/MCP6002 + RC filtr) byla zachována v adresáři `legacy/`.

## Dokumentace

- Kompletní popis projektu: `PROJEKT.md`
- Aktuální zapojení v2: `zapojeni_schema.txt`
- Archivní analogové zapojení: `legacy/zapojeni_schema_v1.txt`

## Versioning

Verze FW je v `rotator_main/version.h` (MAJOR.MINOR.PATCH).  
Při každé mezizměně bumpni `MINOR` (např. 2.1.0 -> 2.2.0).

## Recent Changes

- V2: boot screen + semver v `version.h`
- V2: RX/TX pro 1-wire přes D4/D5 (nekoliduje s TFT)

## Troubleshooting: DATA stuck LOW (~0.5 V)

1. Odpoj bázi tranzistoru na NANO: DATA musí být ~5 V (pull-up OK).
2. Na NANO musí být TX pin v idle HIGH ještě před `linkSerial.begin()`:
   - `pinMode(TX_PIN, OUTPUT);`
   - `digitalWrite(TX_PIN, LOW);` (při použitém NPN a inverted UART)
3. Zkontroluj orientaci C/E tranzistoru a pull-up 4k7 na straně UNO.
