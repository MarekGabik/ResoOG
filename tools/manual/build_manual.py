#!/usr/bin/env python3
"""Builds the ResoOG manual (docs/manual/index.html = CS, en.html = EN).

Style is shared with the ResoQBand manual (tools/manual/head.html). The preset table is read from
Source/State/FactoryPresets.cpp so it always matches the plug-in. Screenshots come from
`ResoOGTests --manual-shots docs/manual/img`.
"""
import re, pathlib, html

ROOT = pathlib.Path(__file__).resolve().parents[2]
VERSION = re.search(r"project\(ResoOG VERSION ([\d.]+)\)", (ROOT / "CMakeLists.txt").read_text()).group(1)
head = (ROOT / "tools/manual/head.html").read_text()
head = re.sub(r"<title>.*?</title>\n", "", head, count=1)

# presets from code: name, category, description
src = (ROOT / "Source/State/FactoryPresets.cpp").read_text()
presets = re.findall(r'\{ "([^"]+)", "([^"]+)", "([^"]+)",', src)

CS_DESC = {
    "Init": "Neutrální start: jedna vrstva se saw, otevřená obálka filtru.",
    "Classic Ladder Bass": "Kulatá analogová basa pro techno a house. Jas = CUTOFF, krátkost = FILTER DECAY.",
    "Deep Blue S+H": "Sharktooth basa, sample & hold v tempu na cutoffu a dub delay.",
    "Sub Foundation": "Čistý sinusový sub s trochou harmonických pro malé reproduktory.",
    "Acid Squelch": "Rezonantní saw s accentem a legato glidem. Hraj překrývané noty.",
    "Reese Wide": "Dva rozladěné saw do šířky, basy drží v mono crossover.",
    "Sync Growl": "Hard sync oscilátoru 2 ovládaný mod obálkou.",
    "Pulse Pluck": "Úzký pulz s pomalým LFO na duty cycle, krátká obálka. Na arpeggia.",
    "Dub Techno Stab": "Obě vrstvy: tmavý stab, chorus na synthu 2 a dlouhý filtrovaný delay.",
    "Techno Rumble": "Saturovaný sinusový sub s tmavým šumem, parťák ke kopáku.",
    "Duo Split": "Duophonic: nižší nota basa, vyšší lead s delayem.",
    "Ambient Drone Bass": "Pomalá trojúhelníková basa, Perlin drift na detune a pomalé LFO.",
    "Perlin Growl": "Sharktooth growl, filtr hýbe plynulý Perlin random.",
    "Bounce Wobble": "Ramp LFO v tempu přes funkci Bounce: wobble, který padá a odskakuje.",
    "Seventies Fat Bass": "Ve stylu Minimoog Model D: dva saw přebuzené v mixeru, kulatý ladder.",
    "Pedal Thunder": "Inspirace Taurus pedály: hluboký saw nad sinusem o dvě oktávy níž.",
    "Multidrive Growl": "Growl ve stylu Sub 37: sharktooth a square tvrdě do mixeru plus drive.",
    "Sync Sweep Bass": "Hard sync jako u Prodigy: mod obálka trhá útok.",
    "Wide Voyager Lead": "Lead jako Voyager: saw a square o oktávu, glide, vibrato na mod wheelu.",
    "Patchable Seq Bass": "Ve stylu Mother-32: rezonantní square na sekvence s accentem a S+H.",
    "Subharmonic Drone": "Podle Subharmonicon: kvinta na osc 2, sub o dvě oktávy, pomalé LFO.",
    "Stereo Matriarch Pad": "Obě vrstvy široce jako stereo Matriarch: chorus, ping-pong, dlouhé obálky.",
    "Phatty Pluck": "Pluck ve stylu Little Phatty se šestnáctinovým delayem s tečkou.",
    "Pedal Sub Pressure": "Čistá váha subu jako Taurus, přes tube saturaci.",
    "Rubber Funk": "Funky basa ze 70. let, gumový filtr a legato slidy.",
    "Octave Pop Bass": "Synth-pop 80. let: osc 2 o oktávu výš pro poskakování.",
    "Berlin Sequence": "Berlínská škola: rezonantní saw, filtr otevírá velmi pomalé LFO, delay.",
    "Detroit Stab": "Krátký square stab, chorus na druhé vrstvě a delay s tečkou.",
    "Warehouse Reese": "Tmavý Reese do skladu: široké saw, mono crossover, tape a kompresor.",
    "Acid Square": "Square acid s accentem a slidy. Automatizuj CUTOFF a RESONANCE.",
    "Acid Distorted": "Saw acid přes tranzistorový drive, LFO hýbe rezonancí.",
    "Dub Sub Wobble": "Měkký trojúhelníkový sub, pomalý wobble v tempu, dlouhý dub delay.",
    "Chord Echo Dub": "Square pluck s kvintou na synthu 2, chorus a dlouhé ozvěny.",
    "Breathing Noise Bass": "Trojúhelník s růžovým šumem na vlastním high passu, Perlin na cutoffu.",
    "Glass Sine Lead": "Sinus a trojúhelník o dvě oktávy, glide, vibrato na mod wheelu.",
    "Velocity Bite": "Slabě = kulatá basa, silně = kousne. Velocity otevírá obálku filtru.",
    "Pressure Swell": "Aftertouch otevírá filtr a přidává vibrato.",
    "Duo Bass & Lead": "Duophonic: spodní nota tlustá basa, horní square lead s glidem.",
    "Triplet Wobble": "Sinusový wobble v osminových triolách na cutoffu.",
}

def preset_rows(lang):
    rows, cat = [], None
    order = ["Basics", "Bass", "Sub", "Classic", "Acid", "Lead", "Pluck", "Techno", "Dub", "Duo", "Ambient"]
    for c in order:
        items = [p for p in presets if p[1] == c]
        if not items:
            continue
        rows.append(f'<tr class="cat"><td colspan="2">{c}</td></tr>')
        for name, _, desc in items:
            d = CS_DESC.get(name, desc) if lang == "cs" else desc
            rows.append(f"<tr><td>{html.escape(name)}</td><td>{html.escape(d)}</td></tr>")
    return "\n        ".join(rows)

def fig(src, alt, cap, w, h, cls="shot", lazy=True):
    return (f'<figure class="{cls}"><img src="img/{src}.jpg" alt="{alt}" width="{w}" height="{h}"'
            + (' loading="lazy"' if lazy else "") + f'><figcaption>{cap}</figcaption></figure>')

def page(lang):
    cs = lang == "cs"
    T = lambda a, b: a if cs else b
    toc = [("princip", T("Jak funguje", "How it works")), ("start", T("Rychlý start", "Quick start")),
           ("synth", "SYNTH"), ("mixer", T("Mixer a drive", "Mixer and drive")), ("filtry", T("Filtry", "Filters")),
           ("cntrl", "CNTRL"), ("modulace", T("Modulace", "Modulation")), ("output", "OUTPUT"),
           ("hrani", T("Hraní a lišty", "Playing and bars")),
           ("bass", "Techno bass"), ("dub", "Dub techno"), ("acid", "Acid"), ("ambient", "Ambient"), ("classic", T("Klasika Moog", "Moog classics")),
           ("presety", T("Tovární presety", "Factory presets")), ("mereni", T("Měření", "Measurements")),
           ("potize", T("Řešení potíží", "Troubleshooting")), ("klavesy", T("Myš a klávesy", "Mouse and keys")),
           ("instalace", T("Instalace a soubory", "Install and files"))]
    genres = {"bass", "dub", "acid", "ambient", "classic"}
    genre_attr = ' class="genre"'
    nl = "\n      "
    toc_html = nl.join(f'<li{genre_attr if k in genres else ""}><a href="#{k}">{v}</a></li>' for k, v in toc)
    lang_sw = (f'<div class="lang" aria-label="{T("Jazyk", "Language")}">'
               + ('<span aria-current="page">CS</span><a href="en.html" hreflang="en" lang="en">EN</a>' if cs
                  else '<a href="index.html" hreflang="cs" lang="cs">CS</a><span aria-current="page">EN</span>') + "</div>")

    body = f'''
<div class="page">
  <nav class="toc" aria-label="{T("Obsah", "Contents")}">
    {lang_sw}
    <p>{T("Obsah", "Contents")}</p>
    <ol>
      {toc_html}
    </ol>
  </nav>

  <main>
  <div class="doc">
    <header class="hero">
      <div class="hero-top">
        <p class="eyebrow">{T("Uživatelská příručka", "User manual")} · {T("verze", "version")} {VERSION}</p>
        {lang_sw}
      </div>
      <h1>Reso<span class="q">OG</span></h1>
      <p class="lede">{T(
        "Dvouvrstvý basový syntezátor. Dva kompletní monofonní synthy na sobě, každý s dual oscilátorem, subem, šumem, přebuzeným mixerem a ladder filtrem, k tomu modulační matice a výstupní efekty. Koncept podle Moog Mariana, zvuk a ovládání v duchu pluginů Gavr.",
        "A dual-layer bass synthesizer. Two complete monophonic synths stacked on top of each other, each with a dual oscillator, sub, noise, an overdriving mixer and a ladder filter, plus a modulation matrix and output effects. Concept after the Moog Mariana, sound and handling in the spirit of the Gavr plug-ins.")}</p>
      <div class="meta">
        <span>VST3 · Windows / macOS</span><span>AU · macOS</span><span>{T("instrument · mono / duophonic", "instrument · mono / duophonic")}</span><span>Ableton Live 11 / 12</span><span>{T("2 vrstvy · 24 modulací · " + str(len(presets)) + " presetů", "2 layers · 24 mod slots · " + str(len(presets)) + " presets")}</span>
      </div>
    </header>

    {fig("overview", T("Stránka SYNTH 1 s presetem Seventies Fat Bass", "SYNTH 1 page with the Seventies Fat Bass preset"),
         T("Stránka <strong>SYNTH 1</strong> s presetem <strong>Seventies Fat Bass</strong>. Nahoře záložky stránek a presety, dole klávesnice. Barvy označují zdroj: oscilátory azurově, sub fialově, šum růžově, mixer oranžově, filtry zlatě.",
           "The <strong>SYNTH 1</strong> page with the <strong>Seventies Fat Bass</strong> preset. Page tabs and presets at the top, keyboard at the bottom. Colours mark the source: oscillators cyan, sub violet, noise pink, mixer orange, filters gold."),
         2400, 1552, lazy=False)}

    <h2 id="princip"><span class="num">01</span>{T("Jak funguje", "How it works")}</h2>
    <p>{T("ResoOG jsou dva stejné syntezátory (vrstvy) poskládané na sebe. Každá vrstva má vlastní zdroje, mixer, filtry a obálky. Stránky se dělí takto:",
          "ResoOG is two identical synthesizers (layers) stacked on top of each other. Each layer has its own sources, mixer, filters and envelopes. The pages are:")}</p>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Stránka", "Page")}</th><th>{T("Co obsahuje", "What it holds")}</th></tr></thead>
      <tbody>
        <tr><td>SYNTH 1 / SYNTH 2</td><td>{T("Zvuk vrstvy: noise, dual oscillator, sub osc, mixer, low pass, high pass, sub filter, crossover, voicing.", "The layer's sound: noise, dual oscillator, sub osc, mixer, low pass, high pass, sub filter, crossover, voicing.")}</td></tr>
        <tr><td>CNTRL 1 / CNTRL 2</td><td>{T("Modulátory vrstvy: 3 LFO, obálka filtru, obálka zesilovače, mod obálka (DAHDSR), 2 random generátory.", "The layer's modulators: 3 LFOs, filter envelope, amp envelope, mod envelope (DAHDSR), 2 random generators.")}</td></tr>
        <tr><td>OUTPUT</td><td>{T("Saturace a efekt každé vrstvy (delay na 1, chorus na 2), summing, FET kompresor, hlasitost a měřiče.", "Saturation and an effect per layer (delay on 1, chorus on 2), summing, FET compressor, volume and meters.")}</td></tr>
      </tbody>
    </table></div>
    <div class="chain"><span>{T("dual osc + sub + noise", "dual osc + sub + noise")}</span><i>→</i><span>mixer (drive)</span><i>→</i><span>LPF + HPF · sub filter</span><i>→</i><span>crossover</span><i>→</i><span>amp</span><i>→</i><span>{T("saturace · delay / chorus", "saturation · delay / chorus")}</span><i>→</i><span class="me">summing → {T("kompresor", "compressor")}</span></div>
    <p>{T("Všechno se zvukem vrstvy běží převzorkovaně (výchozí 2×), takže oscilátory, sync i rezonance zůstávají čisté i ve vysokých polohách. Modulace se počítají každých 32 vzorků a hodnoty se mezi tím plynule interpolují, takže nic necvaká.",
          "Everything in the layer runs oversampled (2× by default), so oscillators, sync and resonance stay clean even high up. Modulation is computed every 32 samples and values glide smoothly in between, so nothing zippers.")}</p>
    <div class="note info"><p><strong>{T("Layered nebo Duophonic", "Layered or Duophonic")}</strong>{T("Vlevo v dolní liště: <em>Layered</em> = obě vrstvy hrají stejnou notu (tlustá basa). <em>Duophonic</em> = synth 1 hraje nejnižší drženou notu, synth 2 nejvyšší (basa + lead jednou rukou).", "In the bottom bar on the left: <em>Layered</em> = both layers play the same note (thick bass). <em>Duophonic</em> = synth 1 plays the lowest held note, synth 2 the highest (bass + lead with one hand).")}</p></div>

    <h2 id="start"><span class="num">02</span>{T("Rychlý start", "Quick start")}</h2>
    <ol class="steps">
      <li><div>{T("Vlož ResoOG na MIDI stopu v Abletonu (Plug-Ins → Gavr → ResoOG).", "Put ResoOG on a MIDI track in Ableton (Plug-Ins → Gavr → ResoOG).")}</div></li>
      <li><div>{T("Klikni na název presetu nahoře a vyber kategorii, nebo procházej šipkami ◀ ▶. U presetu se v menu ukáže popis, k čemu je a co otočit nejdřív.", "Click the preset name at the top and pick a category, or step with the ◀ ▶ arrows. The menu shows what the preset is for and what to turn first.")}</div></li>
      <li><div>{T("Hraj na MIDI klávesy nebo na klávesnici dole. Čím níž klikneš do klávesy, tím silnější úhoz.", "Play your MIDI keyboard or the keyboard at the bottom. The lower you click on a key, the harder the hit.")}</div></li>
      <li><div>{T("Na stránce <strong>SYNTH 1</strong> otoč <strong>CUTOFF</strong>, <strong>RESONANCE</strong> a <strong>EG AMOUNT</strong> v low pass filtru. To je jádro zvuku.", "On the <strong>SYNTH 1</strong> page turn <strong>CUTOFF</strong>, <strong>RESONANCE</strong> and <strong>EG AMOUNT</strong> in the low pass filter. That is the heart of the sound.")}</div></li>
      <li><div>{T("Chceš pohyb? Na stránce <strong>CNTRL 1</strong> chyť šipku v hlavičce LFO a pusť ji na knob. Vznikne modulace.", "Want movement? On the <strong>CNTRL 1</strong> page grab the arrow in an LFO header and drop it on a knob. A modulation is created.")}</div></li>
    </ol>
    <h3 id="vrstvy">{T("Zapnutí a vypnutí vrstev", "Switching layers on and off")}</h3>
    <figure class="strip"><img src="img/layer-power.jpg" alt="{T("Tlačítka napájení vrstev v horní liště", "Layer power buttons in the top bar")}" width="1120" height="72" loading="lazy"></figure>
    <p>{T("Vedle záložek <strong>SYNTH 1</strong> a <strong>SYNTH 2</strong> je tlačítko napájení. Zlaté = vrstva hraje, šedé = vypnutá. Vypnutá vrstva má přeškrtnuté záložky a její stránky jsou ztlumené, ale dál je jde upravovat. Vypnutá vrstva nebere CPU. Stejný stav mají tlačítka MUTE 1 / MUTE 2 na stránce OUTPUT.",
          "Next to the <strong>SYNTH 1</strong> and <strong>SYNTH 2</strong> tabs is a power button. Gold = the layer plays, grey = off. A switched-off layer has its tabs struck through and its pages dimmed, but you can still edit it. An off layer uses no CPU. The MUTE 1 / MUTE 2 buttons on the OUTPUT page show the same state.")}</p>
    <div class="note info"><p><strong>{T("Init preset hraje jen synth 1", "The Init preset plays synth 1 only")}</strong>{T("Aby výchozí zvuk byla jedna čistá vrstva, je synth 2 v Init presetu vypnutý. Zapneš ho tlačítkem vedle záložky SYNTH 2.", "So the default sound is one clean layer, synth 2 is off in the Init preset. Switch it on with the button next to the SYNTH 2 tab.")}</p></div>

    <h2 id="synth"><span class="num">03</span>{T("Stránka SYNTH", "The SYNTH page")}</h2>
    <div style="display:grid;grid-template-columns:minmax(0,1fr) minmax(0,2fr);gap:18px;align-items:start">
      {fig("noise-voicing", "Noise a Voicing", T("NOISE a VOICING", "NOISE and VOICING"), 324, 1096)}
      {fig("oscillators", "Dual oscillator a sub osc", "DUAL OSCILLATOR · SUB OSC", 704, 1096)}
    </div>
    <h3>Dual oscillator</h3>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Ovladač", "Control")}</th><th>{T("Co dělá", "What it does")}</th></tr></thead>
      <tbody>
        <tr><td>FREQUENCY</td><td>{T("Ladění obou oscilátorů ±7 půltónů.", "Tuning of both oscillators, ±7 semitones.")}</td></tr>
        <tr><td>OSC 2 DETUNE</td><td>{T("Rozladění oscilátoru 2 ±7 půltónů. Kolem středu je jemné: +0,05 st dá pomalé zázněje, +7 kvintu.", "Detune of oscillator 2, ±7 semitones. Fine around the centre: +0.05 st gives slow beating, +7 a fifth.")}</td></tr>
        <tr><td>GLIDE</td><td>{T("Klouzání výšky mezi notami (0–10 s). V dolní liště přepneš „Glide: always / legato“.", "Pitch slide between notes (0–10 s). Switch “Glide: always / legato” in the bottom bar.")}</td></tr>
        <tr><td>OSC 2 OCTAVE</td><td>+0 / +1 / +2 {T("oktávy oscilátoru 2.", "octaves for oscillator 2.")}</td></tr>
        <tr><td>HARD SYNC</td><td>{T("Oscilátor 2 začíná znovu s každým cyklem oscilátoru 1. Rozlaď osc 2 (nebo ho moduluj) a dostaneš řezavý sync.", "Oscillator 2 restarts with every cycle of oscillator 1. Detune osc 2 (or modulate it) for the tearing sync sound.")}</td></tr>
        <tr><td>KEY RESET</td><td>{T("Oscilátory začínají každou notu ve stejné fázi: každý úder basy zní stejně průrazně. Vypnuté = volně běžící, živější.", "Oscillators start every note at the same phase: every bass hit has the same punch. Off = free running, more alive.")}</td></tr>
        <tr><td>OSC 2 PHASE</td><td>{T("Fáze osc 2 vůči osc 1. 180° u saw vyruší liché harmonické (zvuk o oktávu výš).", "Phase of osc 2 against osc 1. 180° on a saw cancels the odd harmonics (sounds an octave up).")}</td></tr>
        <tr><td>WAVESHAPE</td><td>{T("Plynulý přechod Sine → Triangle → Sharktooth → Saw → Square. Mezipolohy jsou použitelné a modulovatelné.", "Continuous Sine → Triangle → Sharktooth → Saw → Square. In-between positions are usable and can be modulated.")}</td></tr>
        <tr><td>DUTY CYCLE</td><td>{T("Symetrie každého tvaru, u square šířka pulzu. Výška tónu se nemění.", "Symmetry of every shape, pulse width on the square. Pitch does not change.")}</td></tr>
      </tbody>
    </table></div>
    <h3>Sub osc, Noise, Voicing</h3>
    <ul>
      <li><strong>SUB OSC</strong>: {T("Sine / Saw / Square, FREQ OFFSET −1 oktáva, −1 Linked (sleduje i FREQUENCY a jeho modulaci) nebo −2 oktávy. PHASE posune sub vůči osc 1, když se spodek ztenčuje.", "Sine / Saw / Square, FREQ OFFSET −1 octave, −1 Linked (also follows FREQUENCY and its modulation) or −2 octaves. PHASE shifts the sub against osc 1 if the low end thins out.")}</li>
      <li><strong>NOISE COLOR</strong>: {T("plynule Red (−6 dB/okt) – Pink – White – Blue – Violet (+6 dB/okt).", "continuously Red (−6 dB/oct) – Pink – White – Blue – Violet (+6 dB/oct).")}</li>
      <li><strong>LEGATO MODE</strong>: {T("<em>Re-Trig</em> = obálky začnou s každou notou od nuly (krátké 1,5ms stažení, bez lupnutí). <em>Legato</em> = překrývané noty jen překlouznou. <em>Add</em> = obálky začnou z aktuální úrovně.", "<em>Re-Trig</em> = envelopes start from zero on every note (a short 1.5 ms dip, no click). <em>Legato</em> = overlapping notes only glide. <em>Add</em> = envelopes restart from their current level.")}</li>
      <li><strong>DUAL OSC SPREAD</strong>: {T("osc 1 doleva, osc 2 doprava. S crossoverem zůstanou basy v mono.", "osc 1 left, osc 2 right. With the crossover on, the low end stays mono.")}</li>
      <li><strong>ACCENT</strong>: {T("noty s velocity nad 96 otevřou filtr víc (obálka filtru ×1,5) a ostatní noty se ztiší na 80 %.", "notes with velocity above 96 open the filter more (filter envelope ×1.5) and the others play at 80 %.")}</li>
    </ul>

    <h2 id="mixer"><span class="num">04</span>{T("Mixer a drive", "Mixer and drive")}</h2>
    <div style="display:grid;grid-template-columns:120px minmax(0,1fr);gap:22px;align-items:start">
      <figure class="shot" style="margin:0"><img src="img/mixer.jpg" alt="Mixer" width="264" height="1096" loading="lazy"></figure>
      <div>
        <p>{T("Mixer míchá OSC 1, OSC 2, NOISE a SUB jedné vrstvy (každá vrstva má svůj). Stupnice 0–10 je jako na Moogu: <strong>do 7 je mixer čistý</strong>, nad 7 začne přebuzovat součtový stupeň (červená výseč u knobu). Přebuzení se sčítá: dva oscilátory na 9 kousnou víc než jeden na 10.",
              "The mixer blends OSC 1, OSC 2, NOISE and SUB of one layer (each layer has its own). The 0–10 scale works like on a Moog: <strong>up to 7 the mixer is clean</strong>, above 7 the summing stage starts to overdrive (red arc at the knob). Overdrive adds up: two oscillators at 9 bite more than one at 10.")}</p>
        <p>{T("Ukazatel <strong>DRIVE</strong> pod mixerem ukazuje, jak moc se právě saturuje. Sub má vlastní stupeň a jde mimo hlavní filtry rovnou do sub filtru.",
              "The <strong>DRIVE</strong> meter under the mixer shows how hard it is saturating right now. The sub has its own stage and bypasses the main filters, going straight to the sub filter.")}</p>
        <div class="table-wrap"><table>
          <thead><tr><th>{T("Úroveň (sinus A1)", "Level (sine A1)")}</th><th class="num">{T("3. harmonická", "3rd harmonic")}</th></tr></thead>
          <tbody><tr><td>7</td><td class="num">−47 dB</td></tr><tr><td>10</td><td class="num">−19 dB</td></tr></tbody>
        </table></div>
      </div>
    </div>

    <h2 id="filtry"><span class="num">05</span>{T("Filtry", "Filters")}</h2>
    {fig("filters", T("Low pass, high pass a Filters", "Low pass, high pass and Filters"), T("LOW PASS (24 dB ladder), HIGH PASS (12 dB) a FILTERS s pořadím a crossoverem.", "LOW PASS (24 dB ladder), HIGH PASS (12 dB) and FILTERS with order and crossover."), 1016, 732)}
    <ul>
      <li><strong>LOW PASS</strong>: {T("4pólový ladder filtr (24 dB/okt) se zpětnou vazbou bez zpoždění. Od RESONANCE ~9,5 sám píská, přesně v ladění (odchylka pod 1 cent na 220 Hz až 3 kHz). Úbytek basů při rezonanci je částečně vyrovnaný, jak to známe z Moogu.",
                "4-pole ladder filter (24 dB/oct) with zero-delay feedback. From RESONANCE ~9.5 it self-oscillates in tune (within 1 cent from 220 Hz to 3 kHz). The bass loss at high resonance is partly compensated, the way a Moog behaves.")}</li>
      <li><strong>EG AMOUNT</strong>: {T("kolik obálka filtru (CNTRL) posune cutoff: 10 = 7 oktáv, záporné hodnoty zavírají.", "how far the filter envelope (CNTRL) moves the cutoff: 10 = 7 octaves, negative values close it.")}</li>
      <li><strong>HIGH PASS</strong>: {T("12 dB, s rezonancí a vlastním EG AMOUNT.", "12 dB, with resonance and its own EG AMOUNT.")}</li>
      <li><strong>ORDER</strong>: {T("<em>Serial</em> = LPF a pak HPF (pásmová propust). <em>Parallel</em> = oba sečtené (zářez). <em>HPF Noise</em> = oscilátory jdou do LPF, šum do HPF (čistý spodek a „vzduch“ zvlášť).", "<em>Serial</em> = LPF then HPF (band pass). <em>Parallel</em> = both summed (notch). <em>HPF Noise</em> = oscillators go to the LPF, noise to the HPF (clean low end and separate air).")}</li>
      <li><strong>OSC CROSSOVER + ON/OFF</strong>: {T("doleva = low pass na sub (20 kHz → 40 Hz), doprava = high pass na oscilátory (16 → 500 Hz). Když je zapnutý, maže se i stereo pod 120 Hz: spread zůstane jen ve středech a výškách.", "left = low pass on the sub (20 kHz → 40 Hz), right = high pass on the oscillators (16 → 500 Hz). When on, stereo below 120 Hz is also removed: spread stays in the mids and highs only.")}</li>
    </ul>
    {fig("subfilter", "Sub filter", T("SUB FILTER: vlastní filtr subu (High / Band / Low) s rezonancí a EG AMOUNT.", "SUB FILTER: the sub's own filter (High / Band / Low) with resonance and EG AMOUNT."), 1016, 368, "strip")}

    <h2 id="cntrl"><span class="num">06</span>{T("Stránka CNTRL", "The CNTRL page")}</h2>
    {fig("cntrl", T("Stránka CNTRL 1", "The CNTRL 1 page"), T("CNTRL 1: tři LFO, obálky filtru a zesilovače, mod obálka a dva random generátory. Tečka v hlavičce LFO a random bliká v jejich rytmu.", "CNTRL 1: three LFOs, filter and amp envelopes, the mod envelope and two random generators. The dot in an LFO or random header blinks with it."), 2400, 1552)}
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Modul", "Module")}</th><th>{T("Ovladače", "Controls")}</th></tr></thead>
      <tbody>
        <tr><td>LFO 1–3</td><td>{T("RATE 0,05–50 Hz, nebo se SYNC dělení taktu (8 taktů až 1/64, triolové i s tečkou). KB RESET restartuje LFO s každou notou od fáze PHASE. Tvary Sine, Triangle, Ramp Up, Ramp Down, Square.", "RATE 0.05–50 Hz, or with SYNC a note division (8 bars to 1/64, triplets and dotted). KB RESET restarts the LFO on every note at PHASE. Shapes Sine, Triangle, Ramp Up, Ramp Down, Square.")}</td></tr>
        <tr><td>FILTER ENVELOPE</td><td>{T("ADSR pro EG AMOUNT všech tří filtrů. Attack 1 ms–10 s, decay a release 1 ms–30 s. Attack nabíhá jako RC obvod (rychlý start, měkký konec).", "ADSR for the EG AMOUNT of all three filters. Attack 1 ms–10 s, decay and release 1 ms–30 s. The attack charges like an RC circuit (fast start, soft end).")}</td></tr>
        <tr><td>AMP ENVELOPE</td><td>{T("ADSR hlasitosti vrstvy.", "ADSR of the layer's volume.")}</td></tr>
        <tr><td>MOD ENVELOPE</td><td>{T("DAHDSR (delay, attack, hold, decay, sustain, release) – volná obálka jen pro modulace.", "DAHDSR (delay, attack, hold, decay, sustain, release) – a free envelope for modulation only.")}</td></tr>
        <tr><td>RANDOM 1–2</td><td>{T("<em>S+H</em> = schody, <em>Noise</em> = rychlý náhodný signál, <em>Perlin</em> = plynulý organický drift. SLEW uhlazuje skoky, SYNC hodiny v tempu.", "<em>S+H</em> = steps, <em>Noise</em> = fast random, <em>Perlin</em> = smooth organic drift. SLEW smooths the jumps, SYNC clocks it to the tempo.")}</td></tr>
      </tbody>
    </table></div>
    {fig("lfo-panel", "LFO 1", T("Šipka vpravo v hlavičce je „kabel“: chyť ji a pusť na libovolný knob.", "The arrow on the right of the header is a “patch cable”: grab it and drop it on any knob."), 768, 368, "shot narrow")}

    <h2 id="modulace"><span class="num">07</span>{T("Modulace", "Modulation")}</h2>
    <p>{T("ResoOG má 24 modulačních slotů. Každý vede <strong>zdroj → funkce → controller → amount → cíl</strong>. Cílem může být jakýkoli spojitý knob obou vrstev i výstupu. Zdroje jedné vrstvy mohou modulovat i druhou.",
          "ResoOG has 24 modulation slots. Each one runs <strong>source → function → controller → amount → destination</strong>. Any continuous knob of either layer or the output can be a destination. Sources of one layer can modulate the other.")}</p>
    <h3>{T("Jak vytvořit modulaci", "Creating a modulation")}</h3>
    <ul>
      <li>{T("<strong>Přetažením</strong>: šipka v hlavičce LFO, obálky nebo random → pusť na knob (hloubka 25 %).", "<strong>Drag</strong>: the arrow in an LFO, envelope or random header → drop it on a knob (25 % depth).")}</li>
      <li>{T("<strong>Pravým tlačítkem</strong> na knob → <em>Modulate with</em> → zdroj (i velocity, keyboard, mod wheel, pressure…).", "<strong>Right-click</strong> a knob → <em>Modulate with</em> → source (also velocity, keyboard, mod wheel, pressure…).")}</li>
      <li>{T("V panelu <strong>Mod</strong> (vpravo nahoře nebo klávesa <kbd>9</kbd>) tlačítkem <em>+ Add</em>.", "In the <strong>Mod</strong> panel (top right or the <kbd>9</kbd> key) with <em>+ Add</em>.")}</li>
    </ul>
    <div style="display:grid;grid-template-columns:150px minmax(0,1fr);gap:22px;align-items:center">
      <figure class="shot" style="margin:0"><img src="img/knob-mod.jpg" alt="{T("Knob s modulačním prstencem", "Knob with a modulation ring")}" width="248" height="220" loading="lazy"></figure>
      <p>{T("Vnější <strong>prstenec</strong> v barvě zdroje ukazuje rozsah modulace, bílá tečka její aktuální hodnotu. Najetím na prstenec se ukáže <code>Random 1 ±20 %</code>; tažením prstence nahoru/dolů měníš hloubku, kolečkem po krocích. Pravé tlačítko nabídne Disable, Invert, Bipolar, Remove a otevření slotu v panelu.",
            "The outer <strong>ring</strong> in the source colour shows the modulation range, the white dot its current value. Hover the ring to see <code>Random 1 ±20 %</code>; drag the ring up/down to change the depth, use the wheel for steps. Right-click offers Disable, Invert, Bipolar, Remove and opening the slot in the panel.")}</p>
    </div>
    {fig("mod-panel", T("Panel modulací", "Modulation panel"), T("Panel Mod: seznam všech routingů (klik na tečku = vypnout, × = smazat) a detail vybraného slotu.", "The Mod panel: all routings (click the dot = disable, × = delete) and the details of the selected slot."), 2400, 1552)}
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Volba slotu", "Slot option")}</th><th>{T("Význam", "Meaning")}</th></tr></thead>
      <tbody>
        <tr><td>Bipolar</td><td>{T("Bipolární zdroje (LFO, random, pitch bend) se houpou kolem polohy knobu. Vypnuto = jen jedním směrem.", "Bipolar sources (LFO, random, pitch bend) swing around the knob position. Off = one direction only.")}</td></tr>
        <tr><td>Amount</td><td>{T("Hloubka −100 až +100 % rozsahu knobu. Záporná = obrácená.", "Depth −100 to +100 % of the knob's range. Negative = inverted.")}</td></tr>
        <tr><td>Controller · Ctrl amount</td><td>{T("Druhý zdroj jako „VCA“: modulace je jen tak silná, jak velký je controller. Příklad: LFO → Frequency s controllerem Mod Wheel = vibrato až po vytočení kolečka.", "A second source as a “VCA”: the modulation is only as strong as the controller. Example: LFO → Frequency with Mod Wheel as controller = vibrato only when you push the wheel.")}</td></tr>
        <tr><td>Function · Fn amount</td><td>Scale, Offset, Low / High Clip, Exponential, Norm Exponential, Low / High Pass, Slew, Low / High Slew, Sample &amp; Hold, Bounce.</td></tr>
      </tbody>
    </table></div>
    <p>{T("<strong>Bounce</strong> je fyzika míčku: když signál klesne, hodnota volně padá a odráží se od něj. Na ramp LFO dělá wobble, který dopadá a doznívá (preset Bounce Wobble). <strong>Zdroje</strong>: obálky, LFO, random a accent obou vrstev, velocity, keyboard tracking (49 % na cutoffu = filtr sleduje klávesy 1:1), mod wheel, pressure, pitch bend, release velocity, MPE timbre (CC74) a constant.",
          "<strong>Bounce</strong> is ball physics: when the signal drops, the value falls freely and bounces off it. On a ramp LFO it makes a wobble that lands and settles (preset Bounce Wobble). <strong>Sources</strong>: envelopes, LFOs, randoms and accent of both layers, velocity, keyboard tracking (49 % on cutoff = the filter follows the keys 1:1), mod wheel, pressure, pitch bend, release velocity, MPE timbre (CC74) and constant.")}</p>

    <h2 id="output"><span class="num">08</span>{T("Stránka OUTPUT", "The OUTPUT page")}</h2>
    {fig("output", "OUTPUT", T("OUTPUT s presetem Warehouse Reese: tape saturace na synthu 1, kompresor zapnutý, měřiče ukazují K-14 úroveň, korelaci a gain reduction.", "OUTPUT with the Warehouse Reese preset: tape saturation on synth 1, compressor on, meters show K-14 level, correlation and gain reduction."), 2400, 1552)}
    <h3>{T("Saturace (každá vrstva)", "Saturation (per layer)")}</h3>
    <p>{T("Saturace běží 2× převzorkovaná a má automatickou kompenzaci hlasitosti, takže porovnáváš charakter, ne hlasitost. SAT TYPE <strong>Off</strong> = vypnuto (pak SATURATION nic nedělá).",
          "Saturation runs 2× oversampled with automatic loudness compensation, so you compare character, not level. SAT TYPE <strong>Off</strong> = bypassed (SATURATION then does nothing).")}</p>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Typ", "Type")}</th><th>{T("Jak funguje", "How it works")}</th><th>{T("Zvuk", "Sound")}</th></tr></thead>
      <tbody>
        <tr><td>Tube</td><td>{T("Nesymetrická tanh křivka (záporné půlvlny tvrdší)", "Asymmetric tanh curve (negative half harder)")}</td><td>{T("Sudé harmonické, hřejivé zahuštění", "Even harmonics, warm thickening")}</td></tr>
        <tr><td>Tape</td><td>{T("Měkká symetrická křivka + úbytek výšek s rostoucím drivem", "Soft symmetric curve + high roll-off growing with drive")}</td><td>{T("Kulaté, stlačené, tmavší", "Round, compressed, darker")}</td></tr>
        <tr><td>Drive</td><td>{T("Kubický měkký ořez přecházející v tvrdý", "Cubic soft clip turning hard")}</td><td>{T("Tranzistorové kousnutí, agresivní", "Transistor bite, aggressive")}</td></tr>
      </tbody>
    </table></div>
    <h3>Delay (synth 1) · Chorus (synth 2)</h3>
    <ul>
      <li><strong>Delay</strong>: {T("čas 1 ms–1,4 s nebo v notách (SYNC), FEEDBACK, <strong>DELAY HPF</strong> přímo ve zpětné vazbě (ozvěny nezabahní sub), MIX, DELAY STEREO = ping-pong. Změna času klouže jako u pásky.", "time 1 ms–1.4 s or in note values (SYNC), FEEDBACK, <strong>DELAY HPF</strong> inside the feedback loop (repeats never muddy the sub), MIX, DELAY STEREO = ping-pong. Time changes glide like tape.")}</li>
      <li><strong>Chorus</strong>: {T("RATE, DEPTH, <strong>CHORUS HPF</strong> (chorusuje jen nad touto frekvencí, sub zůstane pevný), MIX, EXPAND = širší stereo.", "RATE, DEPTH, <strong>CHORUS HPF</strong> (only frequencies above it are chorused, the sub stays solid), MIX, EXPAND = wider stereo.")}</li>
    </ul>
    <h3>Summing, {T("kompresor a měřiče", "compressor and meters")}</h3>
    {fig("meters", T("Měřiče", "Meters"), T("IN a OUT ve stupnici K-14 (0 = −14 dBFS, zelená do 0, žlutá do +4, červená nad). Korelace: +1 = mono, kolem +0,5 = zdravé stereo, pod 0 = pozor na mono. Gain reduction kompresoru v dB.", "IN and OUT on the K-14 scale (0 = −14 dBFS, green to 0, yellow to +4, red above). Correlation: +1 = mono, around +0.5 = healthy stereo, below 0 = check mono. Compressor gain reduction in dB."), 1136, 292, "strip")}
    <ul>
      <li>{T("<strong>SYNTH 1/2 LEVEL, PAN, MUTE</strong> a svislý <strong>VOLUME</strong> (dvojklik = 0 dB).", "<strong>SYNTH 1/2 LEVEL, PAN, MUTE</strong> and the vertical <strong>VOLUME</strong> (double-click = 0 dB).")}</li>
      <li>{T("<strong>Kompresor</strong> (tlačítko COMPRESSOR): ATTACK 0,01 ms–1 s, RATIO 1:1–20:1, THRESHOLD s automatickým make-upem, MIX pro paralelní kompresi. <strong>FET</strong> = zpětnovazební detekce a jemné zkreslení jako klasický FET limiter. Release je automatický: delší, když kompresor tlačí víc.", "<strong>Compressor</strong> (COMPRESSOR button): ATTACK 0.01 ms–1 s, RATIO 1:1–20:1, THRESHOLD with automatic make-up, MIX for parallel compression. <strong>FET</strong> = feedback detection and a little grit like a classic FET limiter. Release is automatic: longer when it works harder.")}</li>
    </ul>

    <h2 id="hrani"><span class="num">09</span>{T("Hraní a lišty", "Playing and bars")}</h2>
    {fig("keyboard", T("Klávesnice", "Keyboard"), T("PB (pitch bend, vrací se na střed) a MW (mod wheel), 18 kláves (C1 = MIDI 36 jako v Abletonu), posuvník rozsahu, Hold a oktávy.", "PB (pitch bend, springs back) and MW (mod wheel), 18 keys (C1 = MIDI 36 like Ableton), range slider, Hold and octave buttons."), 2400, 296, "strip")}
    {fig("topbar", T("Horní lišta", "Top bar"), T("Logo, záložky stránek (klávesy 1–5), undo/redo, A/B + Copy, presety, ID instance pro budoucí Virtual CV, Mod, Help (tooltipy, velikost okna 70–150 %, tiché přepínání presetů).", "Logo, page tabs (keys 1–5) with layer power buttons, undo/redo, A/B + Copy, presets, instance ID for the coming Virtual CV, Mod, Help (tooltips, window size 70–150 %, silent preset changes)."), 2400, 72, "strip")}
    {fig("bottombar", T("Dolní lišta", "Bottom bar"), T("Layered / Duophonic, Glide always / legato, rozsah pitch bendu, kategorie a preset, oversampling 1×/2×/4×, CPU instance a špička výstupu.", "Layered / Duophonic, glide always / legato, pitch bend range, category and preset, oversampling 1×/2×/4×, instance CPU and output peak."), 2400, 56, "strip")}
    <p>{T("<strong>Tiché přepínání presetů</strong> (Help, výchozí zapnuto): při změně presetu se výstup na 6 ms ztlumí, preset se nahraje, smažou se ozvěny delaye a staré noty a zvuk se zase plynule vrátí. Žádné lupnutí ani ozvěny starého zvuku v novém.",
          "<strong>Silent preset changes</strong> (Help, on by default): when you change presets the output fades out over 6 ms, the preset loads, delay echoes and old notes are cleared, and the sound fades back in. No clicks and no echoes of the old sound in the new one.")}</p>

    <!-- recipes -->
    <section class="genre-head" id="bass">
      <p class="tempo">125–140 BPM · {T("klub, sub do 40 Hz", "club, sub down to 40 Hz")}</p>
      <h2>Techno bass</h2>
      <p>{T("Basa musí mít čistý mono spodek, hrát s kopákem a mít charakter ve středech.", "The bass needs a clean mono low end, has to sit with the kick, and needs character in the mids.")}</p>
    </section>
    <div class="recipe">
      <div class="recipe-head"><h4>{T("Rolling bass", "Rolling bass")}</h4><span class="where">preset Classic Ladder Bass</span></div>
      <div class="settings"><span class="chip">Waveshape <b>Saw</b></span><span class="chip">Cutoff <b>350–500 Hz</b></span><span class="chip">Resonance <b>3</b></span><span class="chip">EG <b>+5</b></span><span class="chip">Filter decay <b>150–250 ms</b></span><span class="chip">Sustain <b>0 %</b></span></div>
      <div class="recipe-body"><p>{T("Hraj šestnáctiny mezi kopáky. Délku „klapnutí“ nastavuje FILTER DECAY; v rytmu pomůže velocity → EG AMOUNT (preset Velocity Bite). Sub drž na SINE −1 oktáva a nech ho pod kopákem.", "Play sixteenths between the kicks. The length of the “pluck” is FILTER DECAY; for groove add velocity → EG AMOUNT (preset Velocity Bite). Keep the sub on SINE −1 octave under the kick.")}</p></div>
    </div>
    <div class="recipe">
      <div class="recipe-head"><h4>{T("Široká basa, mono spodek", "Wide bass, mono low end")}</h4><span class="where">preset Warehouse Reese</span></div>
      <div class="settings"><span class="chip">Detune <b>+0,15–0,25</b></span><span class="chip">Spread <b>8–9</b></span><span class="chip">Crossover <b>ON, VCO HP ~120 Hz</b></span><span class="chip">Sat <b>Tape 5–6</b></span></div>
      <div class="recipe-body"><p>{T("Crossover vezme oscilátorům spodek a nechá sub čistý a v mono. Korelace na stránce OUTPUT by měla zůstat nad +0,3.", "The crossover takes the low end away from the oscillators and leaves the sub clean and mono. Correlation on the OUTPUT page should stay above +0.3.")}</p></div>
    </div>

    <section class="genre-head" id="dub">
      <p class="tempo">115–125 BPM · {T("delay s dlouhou zpětnou vazbou", "long-feedback delay")}</p>
      <h2>Dub techno</h2>
      <p>{T("Krátké akordové staby rozpuštěné v ozvěnách. Díky HPF ve smyčce delaye ozvěny nezakalí basu.", "Short chord stabs dissolving in echoes. The HPF inside the delay loop keeps the echoes off the bass.")}</p>
    </section>
    <div class="recipe">
      <div class="recipe-head"><h4>{T("Stab jednou rukou", "One-finger stab")}</h4><span class="where">preset Chord Echo Dub / Dub Techno Stab</span></div>
      <div class="settings"><span class="chip">Synth 2 <b>FREQUENCY +7 st</b></span><span class="chip">Amp <b>D 300 ms, S 0</b></span><span class="chip">Delay <b>1/8 D, FB 6–7, HPF 500 Hz</b></span><span class="chip">Chorus <b>40 %</b></span></div>
      <div class="recipe-body"><p>{T("Synth 2 hraje kvintu (FREQUENCY +7), takže jedna klávesa zní jako akord. Automatizuj LPF CUTOFF synthu 1 pomalu přes osm taktů a DELAY FEEDBACK v breaku.", "Synth 2 plays a fifth (FREQUENCY +7), so one key sounds like a chord. Automate synth 1's LPF CUTOFF slowly over eight bars and DELAY FEEDBACK in the break.")}</p></div>
    </div>

    <section class="genre-head" id="acid">
      <p class="tempo">130–145 BPM · {T("rezonance a accent", "resonance and accent")}</p>
      <h2>Acid</h2>
      <p>{T("Squelch vzniká z vysoké rezonance, krátké obálky filtru, accentu a slidů.", "The squelch comes from high resonance, a short filter envelope, accent and slides.")}</p>
    </section>
    <div class="recipe">
      <div class="recipe-head"><h4>{T("Klasická acid linka", "Classic acid line")}</h4><span class="where">preset Acid Squelch / Acid Square</span></div>
      <div class="settings"><span class="chip">Resonance <b>8–8,5</b></span><span class="chip">EG <b>+6,5–7</b></span><span class="chip">Filter decay <b>~170 ms</b></span><span class="chip">Accent <b>ON</b></span><span class="chip">Legato <b>Legato</b></span><span class="chip">Glide <b>60 ms, legato</b></span></div>
      <div class="recipe-body"><p>{T("Noty s velocity nad 96 dostanou accent (víc filtru), překrývané noty překlouznou bez nového úderu obálky. Saturace Drive 5–7 přidá agresi.", "Notes with velocity above 96 get the accent (more filter), overlapping notes slide without retriggering the envelope. Drive saturation 5–7 adds aggression.")}</p></div>
    </div>

    <section class="genre-head" id="ambient">
      <p class="tempo">{T("volné tempo · dlouhé noty", "free tempo · long notes")}</p>
      <h2>Ambient</h2>
      <p>{T("Pomalé obálky, Perlin random a pomalá LFO dělají zvuk, který se nikdy přesně neopakuje.", "Slow envelopes, Perlin random and slow LFOs make a sound that never repeats exactly.")}</p>
    </section>
    <div class="recipe">
      <div class="recipe-head"><h4>{T("Dýchající drone", "Breathing drone")}</h4><span class="where">preset Subharmonic Drone / Breathing Noise Bass</span></div>
      <div class="settings"><span class="chip">Amp <b>A 1–2 s, R 3–4 s</b></span><span class="chip">LFO <b>0,05–0,1 Hz → cutoff 15 %</b></span><span class="chip">Random <b>Perlin 0,2–0,4 Hz → detune</b></span><span class="chip">Delay <b>30 %, FB 6</b></span></div>
      <div class="recipe-body"><p>{T("Drž jednu notu dlouho. Order HPF Noise pošle šum přes vlastní high pass, takže přidává vzduch, ale nekalí spodek.", "Hold one note for a long time. Order HPF Noise sends the noise through its own high pass, so it adds air without clouding the low end.")}</p></div>
    </div>

    <section class="genre-head" id="classic">
      <p class="tempo">{T("inspirace modely Moog", "inspired by Moog models")}</p>
      <h2>{T("Klasika Moog", "Moog classics")}</h2>
      <p>{T("Presety v kategorii Classic napodobují charakter slavných Moogů. Nejsou to modely obvodů, ale nastavení ve stejném duchu.", "The presets in the Classic category imitate the character of famous Moogs. They are not circuit models, but settings in the same spirit.")}</p>
    </section>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Inspirace", "Inspiration")}</th><th>Preset</th><th>{T("Klíč ke zvuku", "Key to the sound")}</th></tr></thead>
      <tbody>
        <tr><td>Minimoog Model D</td><td>Seventies Fat Bass</td><td>{T("dva saw na 8 (mixer přebuzený), ladder ~500 Hz", "two saws at 8 (overdriven mixer), ladder ~500 Hz")}</td></tr>
        <tr><td>Taurus</td><td>Pedal Thunder · Pedal Sub Pressure</td><td>{T("sub −2 oktávy, pomalý decay filtru, glide", "sub −2 octaves, slow filter decay, glide")}</td></tr>
        <tr><td>Sub 37 / Sub Phatty</td><td>Multidrive Growl</td><td>{T("sharktooth + square na 9, Drive saturace", "sharktooth + square at 9, Drive saturation")}</td></tr>
        <tr><td>Prodigy</td><td>Sync Sweep Bass</td><td>{T("hard sync, mod obálka na detune", "hard sync, mod envelope on detune")}</td></tr>
        <tr><td>Voyager</td><td>Wide Voyager Lead</td><td>{T("osc 2 +1 oktáva, glide, vibrato na mod wheelu", "osc 2 +1 octave, glide, vibrato on the mod wheel")}</td></tr>
        <tr><td>Mother-32</td><td>Patchable Seq Bass</td><td>{T("rezonantní square, accent, S+H v tempu", "resonant square, accent, synced S+H")}</td></tr>
        <tr><td>Subharmonicon</td><td>Subharmonic Drone</td><td>{T("kvinta na osc 2, sub −2 oktávy", "a fifth on osc 2, sub −2 octaves")}</td></tr>
        <tr><td>Matriarch</td><td>Stereo Matriarch Pad</td><td>{T("obě vrstvy, chorus + ping-pong, dlouhé obálky", "both layers, chorus + ping-pong, long envelopes")}</td></tr>
        <tr><td>Little Phatty</td><td>Phatty Pluck</td><td>{T("rychle se zavírající saw, delay s tečkou", "fast closing saw, dotted delay")}</td></tr>
      </tbody>
    </table></div>

    <h2 id="presety"><span class="num">10</span>{T("Tovární presety", "Factory presets")}</h2>
    <p>{T(f"{len(presets)} presetů v kategoriích. Klik na název presetu → Factory → kategorie, nebo šipky ◀ ▶.", f"{len(presets)} presets in categories. Click the preset name → Factory → category, or use the ◀ ▶ arrows.")}</p>
    <div class="table-wrap"><table>
      <thead><tr><th>Preset</th><th>{T("Na co a co otočit", "What for and what to turn")}</th></tr></thead>
      <tbody>
        {preset_rows(lang)}
      </tbody>
    </table></div>

    <h2 id="mereni"><span class="num">11</span>{T("Měření", "Measurements")}</h2>
    <p>{T("Hodnoty měří testovací aplikace ResoOGTests při každém buildu (48 kHz).", "Values are measured by the ResoOGTests app on every build (48 kHz).")}</p>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Vlastnost", "Property")}</th><th class="num">{T("Hodnota", "Value")}</th></tr></thead>
      <tbody>
        <tr><td>{T("Aliasing saw B6 (1976 Hz) pod 10 kHz, 1× / 2× / 4×", "Aliasing saw B6 (1976 Hz) below 10 kHz, 1× / 2× / 4×")}</td><td class="num">−78 / −93 / −93 dB</td></tr>
        <tr><td>{T("Aliasing 2×: saw C5 / pulse 25 % / hard sync", "Aliasing 2×: saw C5 / pulse 25 % / hard sync")}</td><td class="num">−97 / −93 / −94 dB</td></tr>
        <tr><td>{T("Výšky oscilátoru (5. harmonická B6) při 2×", "Oscillator top end (5th harmonic of B6) at 2×")}</td><td class="num">−0,9 dB</td></tr>
        <tr><td>{T("Ladder samooscilace 220 / 1000 / 3000 Hz", "Ladder self-oscillation 220 / 1000 / 3000 Hz")}</td><td class="num">&lt; 1 cent</td></tr>
        <tr><td>{T("CPU: 1 vrstva 2× / 2 vrstvy + efekty 2× / 2 vrstvy 4×", "CPU: 1 layer 2× / 2 layers + effects 2× / 2 layers 4×")}</td><td class="num">~1,8 / ~3,4 / ~6 %</td></tr>
        <tr><td>{T("Latence (2× vrstva + 2× saturace) při 48 kHz", "Latency (2× layer + 2× saturation) at 48 kHz")}</td><td class="num">{T("pár vzorků, hlásí se Live", "a few samples, reported to Live")}</td></tr>
      </tbody>
    </table></div>

    <h2 id="potize"><span class="num">12</span>{T("Řešení potíží", "Troubleshooting")}</h2>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Problém", "Problem")}</th><th>{T("Řešení", "Fix")}</th></tr></thead>
      <tbody>
        <tr><td>{T("Synth 2 / CNTRL 2 nic nedělá", "Synth 2 / CNTRL 2 does nothing")}</td><td>{T("Vrstva je vypnutá (přeškrtnutá záložka). Zapni ji tlačítkem napájení vedle záložky SYNTH 2.", "The layer is off (struck-through tab). Switch it on with the power button next to the SYNTH 2 tab.")}</td></tr>
        <tr><td>{T("SATURATION nic nedělá", "SATURATION does nothing")}</td><td>{T("SAT TYPE je Off. Zvol Tube, Tape nebo Drive.", "SAT TYPE is Off. Pick Tube, Tape or Drive.")}</td></tr>
        <tr><td>{T("Knoby delaye / chorusu nic nedělají", "Delay / chorus knobs do nothing")}</td><td>{T("DELAY MIX / CHORUS MIX je na 0 %.", "DELAY MIX / CHORUS MIX is at 0 %.")}</td></tr>
        <tr><td>{T("Kompresor nic nedělá", "The compressor does nothing")}</td><td>{T("Zapni tlačítko COMPRESSOR.", "Turn on the COMPRESSOR button.")}</td></tr>
        <tr><td>OSC CROSSOVER</td><td>{T("Pracuje jen se zapnutým ON/OFF pod ním.", "Works only with ON/OFF below it switched on.")}</td></tr>
        <tr><td>{T("PHASE u LFO nic nemění", "LFO PHASE changes nothing")}</td><td>{T("PHASE určuje, odkud LFO startuje při KB RESET (nebo v tempu se SYNC).", "PHASE sets where the LFO starts on KB RESET (or in time with SYNC).")}</td></tr>
        <tr><td>{T("Glide nefunguje", "Glide does not work")}</td><td>{T("V dolní liště je „Glide: legato“: glide je jen mezi překrývanými notami.", "The bottom bar says “Glide: legato”: glide happens only between overlapping notes.")}</td></tr>
        <tr><td>{T("Šum nebo pískání ve výškách", "Noise or whistling up high")}</td><td>{T("Oversampling 1× v dolní liště; přepni na 2× nebo 4×.", "Oversampling 1× in the bottom bar; switch to 2× or 4×.")}</td></tr>
      </tbody>
    </table></div>

    <h2 id="klavesy"><span class="num">13</span>{T("Myš a klávesy", "Mouse and keys")}</h2>
    <div class="table-wrap"><table>
      <thead><tr><th>{T("Akce", "Action")}</th><th>{T("Výsledek", "Result")}</th></tr></thead>
      <tbody>
        <tr><td>{T("Tažení knobu nahoru/dolů", "Drag a knob up/down")}</td><td>{T("Změna hodnoty, <kbd>Shift</kbd> = jemně", "Change the value, <kbd>Shift</kbd> = fine")}</td></tr>
        <tr><td>{T("Kolečko nad knobem", "Wheel over a knob")}</td><td>{T("Krok (u přepínačů jedna volba)", "Step (one choice on switches)")}</td></tr>
        <tr><td>{T("Dvojklik na knob", "Double-click a knob")}</td><td>{T("Napsat hodnotu: <code>2.5k</code>, <code>A3</code>, <code>C#3+20</code>, <code>120 ms</code>, <code>2s</code>, <code>L 30</code>", "Type a value: <code>2.5k</code>, <code>A3</code>, <code>C#3+20</code>, <code>120 ms</code>, <code>2s</code>, <code>L 30</code>")}</td></tr>
        <tr><td><kbd>Cmd</kbd>/<kbd>Ctrl</kbd>-{T("klik", "click")}</td><td>{T("Výchozí hodnota", "Default value")}</td></tr>
        <tr><td>{T("Pravé tlačítko na knob", "Right-click a knob")}</td><td>{T("Reset, napsat, Modulate with, správa modulací", "Reset, type, Modulate with, manage modulations")}</td></tr>
        <tr><td>{T("Tažení prstence knobu", "Drag a knob's ring")}</td><td>{T("Hloubka modulace", "Modulation depth")}</td></tr>
        <tr><td>{T("Šipka v hlavičce CNTRL → knob", "CNTRL header arrow → knob")}</td><td>{T("Nová modulace", "New modulation")}</td></tr>
        <tr><td><kbd>1</kbd>–<kbd>5</kbd></td><td>SYNTH 1, CNTRL 1, SYNTH 2, CNTRL 2, OUTPUT</td></tr>
        <tr><td><kbd>9</kbd> / <kbd>0</kbd>, <kbd>Esc</kbd></td><td>{T("Panel modulací otevřít / zavřít", "Open / close the modulation panel")}</td></tr>
        <tr><td><kbd>Cmd</kbd>+<kbd>Z</kbd>, <kbd>Shift</kbd>+<kbd>Cmd</kbd>+<kbd>Z</kbd></td><td>Undo / Redo</td></tr>
      </tbody>
    </table></div>

    <h2 id="instalace"><span class="num">14</span>{T("Instalace a soubory", "Install and files")}</h2>
    <div class="table-wrap"><table>
      <thead><tr><th></th><th>macOS</th><th>Windows</th></tr></thead>
      <tbody>
        <tr><td>VST3</td><td><code>~/Library/Audio/Plug-Ins/VST3/ResoOG.vst3</code></td><td><code>C:\\Program Files\\Common Files\\VST3\\ResoOG.vst3</code></td></tr>
        <tr><td>AU</td><td><code>~/Library/Audio/Plug-Ins/Components/ResoOG.component</code></td><td>–</td></tr>
        <tr><td>{T("Uživatelské presety", "User presets")}</td><td><code>~/Documents/Gavr/ResoOG Presets/*.resoog</code></td><td><code>Documents\\Gavr\\ResoOG Presets\\*.resoog</code></td></tr>
        <tr><td>{T("Nastavení (tooltipy, velikost)", "Settings (tooltips, size)")}</td><td><code>~/Library/Application Support/Gavr/ResoOG/</code></td><td><code>%APPDATA%\\Gavr\\ResoOG\\</code></td></tr>
      </tbody>
    </table></div>
    <p>{T("V Abletonu: Settings → Plug-Ins → zapni „Use VST3 Plug-In System Folders“ → Rescan. ResoOG najdeš pod Plug-Ins → Gavr.", "In Ableton: Settings → Plug-Ins → turn on “Use VST3 Plug-In System Folders” → Rescan. ResoOG is under Plug-Ins → Gavr.")}</p>

    <footer>ResoOG · {T("verze", "version")} {VERSION} · Gavr. {T("Moog, Minimoog, Taurus, Mariana a názvy modelů jsou ochranné známky Moog Music Inc.; ResoOG s nimi nesouvisí, inspiruje se jen konceptem. VST je ochranná známka Steinberg Media Technologies GmbH.", "Moog, Minimoog, Taurus, Mariana and the model names are trademarks of Moog Music Inc.; ResoOG is not affiliated with them and only borrows the concept. VST is a trademark of Steinberg Media Technologies GmbH.")}</footer>
  </div>
  </main>
</div>
'''
    title = "<title>ResoOG Příručka</title>\n" if cs else "<title>ResoOG Manual</title>\n"
    return title + head + body

out = ROOT / "docs/manual"
(out / "index.html").write_text(page("cs"))
# en.html is served as a separate page: give it its own document skeleton (index.html gets one when published)
skeleton = ('<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">'
            '<style>:root{color-scheme:light;box-sizing:border-box;padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)}'
            'body{margin:0}img{max-width:100%}[hidden]{display:none!important}</style></head><body>\n')
(out / "en.html").write_text(skeleton + page("en") + "\n</body></html>\n")
print("written", out / "index.html", out / "en.html", len(presets), "presets")
