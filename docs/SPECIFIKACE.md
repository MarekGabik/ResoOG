# ResoOG – specifikace a plán vývoje

Výchozí zadání: *ResoOG Software Synthesizer – Plugin Design Specification* (koncept podle Moog Mariana) +
*Gavr plugin blueprint* (postup, stack a vizuální jazyk rodiny Gavr). Tento dokument popisuje, co je skutečně postavené,
jak, s jakými změřenými hodnotami, a co následuje.

## 1. Shrnutí

Monofonní dvouvrstvý basový syntezátor. Každá vrstva = dual oscillator + sub + noise → overdrive mixer → ladder LPF + HPF
(+ sub filter, crossover) → VCA, s vlastní stránkou modulátorů (CNTRL). Výstupní stránka: saturace a efekty pro každou vrstvu,
summing, FET kompresor a měřiče. Formáty VST3 (Win/Mac), AU (Mac), Standalone; cíl Ableton Live 11/12.

## 2. Stack

JUCE 8.0.10, C++17, CMake + Ninja, universal binary arm64 + x86_64, macOS 10.13+. `COMPANY Gavr`, `BUNDLE_ID cz.gavr.resoog`,
`PLUGIN_CODE RsOG`, instrument (`aumu`). CI: GitHub Actions Windows + macOS (build, testy, artefakt VST3).

## 3. Architektura kódu

```
Source/
  Params/Parameters.*     tabulky parametrů (vrstva 75, globální 35, modulační slot 9 × 24), formát a parsování hodnot,
                          seznamy zdrojů / cílů / funkcí modulace, indexy LP:: / GP:: / MP::
  DSP/Primitives.h        ShapeOsc (PolyBLEP/BLAMP s jednovzorkovým zpožděním, morf tvarů, duty, hard sync), Ladder (ZDF, saturace na vstupu),
                          SVF, LR4, šum s barvou, obálka DAHDSR (RC attack), LFO, Random (S+H / Noise / Perlin + slew)
  DSP/SynthLayer.*        jedna vrstva, audio na převzorkované frekvenci, modulátory na řídicí frekvenci
  DSP/ModMatrix.*         24 slotů: zdroj → funkce → controller (VCA) → amount → cíl (normalizovaný offset)
  DSP/Effects.*           saturátor, stereo delay, chorus, FET kompresor, měřiče K-14 / korelace
  State/                  StateHistory (undo/redo + A/B, z ResoQBandu), GlobalSettings, FactoryPresets (v kódu)
  UI/                     Theme (paleta Gavr + tokeny panelu), Knob, Widgets (LED tlačítka, přepínače, panely se štítkem),
                          Pages (SYNTH / CNTRL / OUTPUT), Keyboard, Bars (horní a dolní lišta), ModDrawer
tools/TestHost/Main.cpp   testy, --bench
```

Zpracování v `processBlock`: blok se dělí na kousky ≤ 32 vzorků a navíc přesně v místech MIDI událostí (časování na vzorek).
Pro každý kousek: zdroje modulace → matice → modulované hodnoty parametrů → vrstvy (převzorkování nahoru, render, dolů)
→ DC blokátor → efekty vrstev → summing → kompresor → hlasitost → měřiče. Audio vlákno nealokuje, nezamyká, nepracuje s řetězci;
latence oversamplingu se hlásí přes `AsyncUpdater`. Spojité hodnoty se uvnitř kousku interpolují lineárně (bez zipperu).

## 4. UI

Logická velikost 1200 × 826, škálování 70–150 % (pevný poměr stran). Horní lišta (36 px): logo, 5 záložek, undo/redo, A/B, Copy,
preset ◀ název ▶, CV ID instance, Mod, Help. Panel ve stylu Mariany: světlé štítky modulů s barevnou linkou, ořechové boky,
knoby se stupnicí a kovovou hlavou, skleněná zlatá tlačítka, LED přepínače. Barvy nesou informaci: oscilátory azurové,
sub fialový, šum růžový, mixer oranžový, filtry zlaté, voicing zelený; každý modulační zdroj má vlastní barvu (prstenec na cíli).

Interakce: svislé tažení, Shift = jemně, kolečko, dvojklik = psaní hodnoty (`2.5k`, `A3`, `C#3+20`, `120 ms`, `L 30`),
Cmd/Ctrl-klik = výchozí, pravé tlačítko = menu (reset, modulace, editace slotu). Šipku v hlavičce LFO / obálky / random
přetáhni na knob → modulace 25 %; prstenec kolem knobu se táhne (hloubka), při najetí ukáže `LFO 1 ±20 %`, bílá tečka = živá hodnota.
Panel Mod: seznam všech slotů a detail (zdroj, cíl, bipolar, amount, controller, funkce). Klávesy 1–5 stránky, 9 panel Mod.

## 5. DSP – rozhodnutí

| Blok | Řešení |
|---|---|
| Oscilátory | analytické tvary s duty (warp fáze), korekce skoků (BLEP) i zlomů (BLAMP) na obou stranách události; hard sync korektně band-limitovaný |
| Oversampling | JUCE polyphase IIR 2×/4× pro celou vrstvu (oscilátory, mixer, filtry, VCA); 2× výchozí |
| Mixer | lineární zisk, kanály nad 7 zvyšují přebuzení součtového stupně (měkký saturátor), sub má vlastní stupeň |
| LPF | 4pólový ZDF ladder, nelinearita na vstupu zpětné vazby, kompenzace úbytku basů (1 + k/2), samooscilace od ~9,5 |
| HPF / Sub filter | TPT SVF, Q 0,5–12 |
| Crossover | LR4: low pass na sub (40 Hz–20 kHz), nebo high pass na oscilátory (16–500 Hz); side pod 120 Hz se maže (mono basy) |
| Obálky | RC attack (cíl 1,3), exponenciální decay/release; Re-Trig = 1,5ms stažení na nulu → bez lupnutí |
| Modulace | po kouscích ≤ 32 vzorků v normalizovaném rozsahu cíle; funkce se stavem (slew, LP, S&H, bounce) |
| Efekty | saturace na základní frekvenci; delay s HPF a tlumením 9 kHz ve smyčce; chorus jen nad nastaveným HPF |
| Kompresor | FET = zpětnovazební detekce + jemné zkreslení, release automatický (120 ms, delší při velké kompresi) |

## 6. Testy a měření (ResoOGTests, 77 kontrol, vše prochází)

- parametry: pořadí tabulek vs. konstanty, 401 unikátních ID, formát/parsování, kódy cílů modulace
- výchozí patch: rozumná úroveň (−4,7 dBFS), ticho před notou, doznění do ticha
- aliasing (saw B6 při 48 kHz, nejsilnější alias pod 10 kHz): 1× −53 dB, 2× −70 dB, 4× −84 dB; saw C5 2× −81 dB; pulse 25 % 2× −71 dB; hard sync 2× −78 dB
- mixer: úroveň 7 → 3. harmonická −47 dB, úroveň 10 → −19 dB
- ladder: samooscilace 220/1000/3000 Hz s odchylkou < 1 cent
- glide, duophonic přidělení, priorita poslední noty, sustain, Re-Trig bez lupnutí
- modulace: LFO → cutoff (výkyv 63 dB), všechny funkce konečné, controller jako VCA, mazání slotů
- robustnost: 44,1/96/192 kHz × bloky 1–4096, prázdný blok, vše na maximu
- stav: uložení / obnovení všech parametrů a názvu presetu
- 14 továrních presetů hraje (špičky −2,9 až −13 dBFS)
- CPU: 1 vrstva 2× ~1,5 %, 2 vrstvy + efekty 2× ~3,1 %, 2 vrstvy 4× ~5,9 % jádra; nečinnost (denormaly) < 0,5 %
- UI: snímky všech stránek, tažení knobu, upuštění modulačního zdroje, psaní hodnoty, panel Mod, velikost okna
- hostování sestaveného VST3 (instrument, hraje notu)

## 7. Rozdíly proti zadání

- **Virtual CV** (8 × 8 mezi instancemi): zatím jen generované ID instance v liště; přijde ve fázi 3.
- **MPE**: CC74 (timbre) a pressure fungují jako globální zdroje; plné MPE per-note ve fázi 3.
- **Klávesnice**: místo „Correction“ a „Scale“ jsou zatím tlačítka oktáv a Hold; scale lock a ribbon režim ve fázi 2.
- **Silent preset changes**: zatím ne.
- Chorus rate 0,05–20 Hz (zadání 0,5–50 Hz – nad 20 Hz už to není chorus); kompresor má automatický release (zadání ho neuvádí).
- Přidáno: oversampling 1×/2×/4×, ukazatel drive mixeru, glide jen pro legato, rozsah pitch bendu.

## 8. Roadmapa s branami

| Fáze | Obsah | Brána (musí projít) |
|---|---|---|
| 0.1 ✅ | engine obou vrstev, matice, efekty, UI všech stránek, presety, CI | všechny testy výše, universal build, auval, VST3 v Live |
| 0.2 | poslech a doladění zvuku v Live (charakter ladderu, mixer, obálky), další presety (cíl 40), silent preset change, scale lock klávesnice | null/regresní testy presetů, CPU ≤ současné +20 % |
| 0.3 | MPE per-note, Virtual CV mezi instancemi (sdílená paměť, 8 × 8), modulace z CV IN | test dvou instancí v jednom procesu, latence CV ≤ 1 blok |
| 0.4 | oversampling efektů, vylepšený oscilátor (vyšší řád BLEP) pro 2× < −80 dB i na B6 | aliasing test 2× > 80 dB |
| 1.0 | manuál CS/EN se screenshoty (`--manual-shots`), audit vláken/paměti, podepsání a notarizace | audit, testy na Windows i Mac zelené |
