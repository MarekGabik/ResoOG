# ResoOG

Dvouvrstvý basový syntezátor (VST3, na macOS i AU) pro Ableton Live na macOS a Windows.
Koncept a rozložení panelů vychází z Moog Mariana (dvě vrstvy SYNTH 1/2, ovládací stránky CNTRL 1/2,
výstupní stránka OUTPUT). Kód, grafika a název jsou vlastní, vzhled patří do rodiny pluginů Gavr
(ResoQBand, ResoTamer): klidné tmavé prostředí, barva jen tam, kde nese informaci.

Specifikace a plán vývoje: [docs/SPECIFIKACE.md](docs/SPECIFIKACE.md) · vzhledový mockup: [design/resoog-mockup.html](design/resoog-mockup.html)

## Stav (verze 0.1.0 – první hratelná verze)

Hotovo a otestováno:

- **2 nezávislé vrstvy**, režim Layered (obě hrají stejnou notu) nebo Duophonic (vrstva 1 = nejnižší, vrstva 2 = nejvyšší držená nota),
  priorita poslední noty, sustain pedál, tlačítko Hold, glide (vždy / jen legato), pitch bend ±1–24
- **Dual oscillator**: plynulý tvar Sine → Triangle → Sharktooth → Saw → Square, duty cycle na všech tvarech,
  Frequency ±7 st, Detune OSC 2 (jemný kolem středu), Osc 2 Octave +0/+1/+2, Osc 2 Phase, Hard Sync, Key Reset
- oscilátory bez aliasingu (PolyBLEP/BLAMP i pro hard sync) + oversampling 1×/2×/4× (latence se hlásí hostu)
- **Sub oscillator** (Sine/Saw/Square, −1 Oct / −1 Linked / −2 Oct, fáze), **Noise** s plynulou barvou Red–Pink–White–Blue–Violet
- **Mixer ve stylu CP-3**: do 7 čistý, nad 7 přebuzení součtového stupně (ukazatel DRIVE)
- **24 dB ladder LPF** (samooscilace, ladění přesné na ±1 cent), **12 dB HPF**, pořadí Serial / Parallel / HPF Noise,
  **Sub filter** HP/BP/LP, **Osc crossover** (low pass na sub, nebo high pass na oscilátory) s mono basy
- **CNTRL**: 3 LFO (5 tvarů, sync na tempo, KB reset, fáze), Filter a Amp ADSR, Mod DAHDSR, 2× Random (S+H, Noise, Perlin, slew, sync),
  legato režimy Re-Trig (bez lupnutí) / Legato / Add, Accent (velocity > 96)
- **Modulační matice**: 24 slotů, 27 zdrojů (vrstvové modulátory obou vrstev, velocity, keyboard, mod wheel, pressure, pitch bend,
  release velocity, MPE timbre CC74, constant), libovolný spojitý parametr jako cíl, bipolar/unipolar, controller jako VCA,
  14 funkcí (Scale, Offset, Clip, Exp, Low/High Pass, Slew, S&H, Bounce…)
- **OUTPUT**: saturace Tube/Tape/Drive na každé vrstvě, delay vrstvy 1 (HPF ve zpětné vazbě, ping-pong, sync),
  chorus vrstvy 2 (jen výšky, expand), summing (level, pan, mute), FET kompresor (attack, ratio, threshold s automatickým make-up, mix),
  hlasitost, měřiče K-14 IN/OUT, korelace a gain reduction
- **UI**: 5 stránek jako mockup, knoby s prstencem modulace (tažením prstence se mění hloubka), přetažení šipky z hlavičky LFO/obálky
  na knob = nová modulace, pravé tlačítko na knobu = menu modulací, dvojklik = napsat hodnotu (i `2.5k`, `A3`, `120 ms`),
  Cmd-klik = výchozí hodnota, Shift = jemně, panel Mod (souhrn a editor všech modulací), klávesnice na obrazovce
  (velocity podle místa úhozu, PB/MW, oktávy, Hold), undo/redo, A/B, presety, velikost okna 70–150 %
- **14 továrních presetů** v 9 kategoriích (Bass, Sub, Acid, Lead, Pluck, Dub, Techno, Duo, Ambient), uložení/načtení vlastních (`.resoog`)
- 401 automatizovatelných parametrů se stabilními ID

Naměřeno (`ResoOGTests`, MacBook M-series, 48 kHz):

| Vlastnost | Hodnota |
|---|---|
| Aliasing saw B6 (1976 Hz) pod 10 kHz | 1×: −53 dB, 2×: −70 dB, 4×: −84 dB |
| Aliasing saw C5 (523 Hz), 2× | −81 dB |
| Hard sync 2× / pulse 25 % 2× | −78 dB / −71 dB |
| Ladder samooscilace 220 / 1000 / 3000 Hz | odchylka < 1 cent |
| Mixer úroveň 7 / 10 (3. harmonická) | −47 dB (čistý) / −19 dB (drive) |
| CPU 1 vrstva 2× / 2 vrstvy + efekty 2× / 2 vrstvy 4× | ~1,5 % / ~3,1 % / ~5,9 % jednoho jádra |

Plánováno (viz roadmapa ve specifikaci): Virtual CV mezi instancemi (ID je vidět vpravo nahoře), MPE per-note,
scale lock a ribbon pitch correction na klávesnici, manuál CS/EN, další presety.

## Build (macOS)

Potřeba: CMake ≥ 3.22, Ninja, Xcode Command Line Tools. Složka `JUCE/` (JUCE 8.0.10) leží lokálně v projektu
a není v gitu; když chybí, CMake si JUCE stáhne sám.

```bash
cmake -B build-universal -G Ninja -DCMAKE_BUILD_TYPE=Release -DRESOOG_TESTS=ON
cmake --build build-universal --target ResoOG_VST3 ResoOG_AU ResoOGTests
```

Po buildu se plugin sám nainstaluje do `~/Library/Audio/Plug-Ins/VST3/ResoOG.vst3` a `Components/ResoOG.component`
(universal binary arm64 + x86_64, macOS 10.13+).

Testy (zvuk, UI snímky do `snapshots/`, hostování nainstalovaného VST3):

```bash
build-universal/ResoOGTests_artefacts/Release/ResoOGTests ~/Library/Audio/Plug-Ins/VST3/ResoOG.vst3
```

Měření výkonu: `ResoOGTests --bench`.

## Windows

Windows verzi staví GitHub Actions (`.github/workflows/build.yml`) při každém pushi: Windows + macOS build, spuštění testů
a artefakt **ResoOG-VST3-Windows** ke stažení (záložka Actions → poslední běh → Artifacts).
Obsah artefaktu (`ResoOG.vst3`) zkopíruj do `C:\Program Files\Common Files\VST3\`.

## Ableton Live

1. Po buildu (nebo zkopírování na Windows) otevři Live → Settings → Plug-Ins → zapni „Use VST3 Plug-In System Folders“ → Rescan.
2. ResoOG najdeš v prohlížeči pod Plug-Ins → Gavr → ResoOG (instrument). Přetáhni na MIDI stopu.
3. Presety: klik na název presetu nahoře. Stránky přepínáš záložkami nebo klávesami 1–5, panel modulací klávesou 9.

## Licence

Kód © Gavr. JUCE je použit pod licencí JUCE 8 (AGPLv3 / komerční). Moog a Mariana jsou ochranné známky Moog Music Inc.;
ResoOG s nimi nesouvisí, inspiruje se pouze konceptem.
