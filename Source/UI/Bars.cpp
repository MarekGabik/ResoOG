#include "Bars.h"
#include "PluginProcessor.h"
#include "State/FactoryPresets.h"

using namespace juce;

const StringArray& TopBar::tabNames()
{
    static const StringArray t { "SYNTH 1", "CNTRL 1", "SYNTH 2", "CNTRL 2", "OUTPUT" };
    return t;
}

TopBar::TopBar (ResoOGProcessor& p, GlobalSettings& s) : proc (p), settings (s)
{
    if (proc.history) proc.history->addChangeListener (this);
}

TopBar::~TopBar()
{
    if (proc.history) proc.history->removeChangeListener (this);
}

Rectangle<float> TopBar::area (int item) const
{
    const float h = (float) height;
    if (item >= Tab0 && item < Tab0 + 5)
    {
        float x = 92.0f;
        for (int i = 0; i < item; ++i) x += (float) GlyphArrangement::getStringWidthInt (Fonts::bold (11.0f), tabNames()[i]) + 26.0f;
        return { x, 0.0f, (float) GlyphArrangement::getStringWidthInt (Fonts::bold (11.0f), tabNames()[item]) + 22.0f, h };
    }
    const float cx = getWidth() * 0.5f + 220.0f;
    const float x0 = area (Tab0 + 4).getRight() + 16.0f;
    switch (item)
    {
        case Undo: return { x0, 8.0f, 20.0f, 20.0f };
        case Redo: return { x0 + 26.0f, 8.0f, 20.0f, 20.0f };
        case AB:   return { x0 + 58.0f, 6.0f, 32.0f, 24.0f };
        case Copy: return { x0 + 94.0f, 6.0f, 38.0f, 24.0f };
        case Prev: return { cx - 140.0f - 24.0f, 6.0f, 20.0f, 24.0f };
        case Name: return { cx - 140.0f, 6.0f, 210.0f, 24.0f };
        case Next: return { cx + 74.0f, 6.0f, 20.0f, 24.0f };
        case CV:   return { (float) getWidth() - 214.0f, 8.0f, 62.0f, 20.0f };
        case Mod:  return { (float) getWidth() - 142.0f, 6.0f, 42.0f, 24.0f };
        case Help: return { (float) getWidth() - 94.0f, 6.0f, 40.0f, 24.0f };
        default:   return {};
    }
}

int TopBar::itemAt (Point<float> p) const
{
    for (int i = 0; i < 5; ++i) if (area (i).contains (p)) return i;
    for (int i = Undo; i <= Help; ++i) if (area (i).contains (p)) return i;
    return None;
}

void TopBar::refresh()
{
    const auto n = proc.getPresetName();
    if (n != shownName) { shownName = n; repaint(); }
}

void TopBar::paint (Graphics& g)
{
    g.fillAll (Palette::bar);
    g.setColour (Colour (0xff16171b));
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

    // logo
    g.setFont (Fonts::bold (16.0f));
    g.setColour (Palette::text);
    g.drawText ("Reso", 14, 0, 60, height, Justification::centredLeft, false);
    g.setColour (Palette::curve);
    g.drawText ("OG", 14 + GlyphArrangement::getStringWidthInt (Fonts::bold (16.0f), "Reso"), 0, 40, height, Justification::centredLeft, false);

    // tabs
    for (int i = 0; i < 5; ++i)
    {
        const auto r = area (i);
        const bool on = i == page;
        g.setColour (on ? Palette::text : (hover == i ? Palette::textDim : Palette::textFaint));
        g.setFont (Fonts::bold (11.0f).withExtraKerningFactor (0.06f));
        g.drawText (tabNames()[i], r, Justification::centred, false);
        if (on)
        {
            g.setColour (Palette::curve);
            g.fillRoundedRectangle (r.getX() + 11.0f, r.getBottom() - 5.0f, r.getWidth() - 22.0f, 2.0f, 1.0f);
        }
    }

    auto* h = proc.history.get();
    Icons::draw (g, Icons::undo(), area (Undo).reduced (2.0f), Palette::textDim.withAlpha (h && h->canUndo() ? (hover == Undo ? 1.0f : 0.8f) : 0.3f));
    Icons::draw (g, Icons::redo(), area (Redo).reduced (2.0f), Palette::textDim.withAlpha (h && h->canRedo() ? (hover == Redo ? 1.0f : 0.8f) : 0.3f));

    g.setFont (Fonts::bold (12.0f));
    const int slot = h ? h->getActiveSlot() : 0;
    auto abR = area (AB);
    g.setColour (slot == 0 ? Palette::text : Palette::textDim);
    g.drawText ("A", abR.withWidth (12.0f).translated (2.0f, 0.0f), Justification::centred, false);
    g.setColour (Palette::textFaint);
    g.drawText ("/", abR.withWidth (8.0f).translated (12.0f, 0.0f), Justification::centred, false);
    g.setColour (slot == 1 ? Palette::text : Palette::textDim);
    g.drawText ("B", abR.withWidth (12.0f).translated (18.0f, 0.0f), Justification::centred, false);
    g.setFont (Fonts::regular (12.0f));
    g.setColour (hover == Copy ? Palette::text : Palette::textDim);
    g.drawText ("Copy", area (Copy), Justification::centred, false);

    // preset
    Icons::draw (g, Icons::chevron (false), area (Prev).reduced (5.0f, 7.0f), hover == Prev ? Palette::text : Palette::textDim);
    Icons::draw (g, Icons::chevron (true), area (Next).reduced (5.0f, 7.0f), hover == Next ? Palette::text : Palette::textDim);
    auto nr = area (Name);
    g.setColour (hover == Name ? Palette::buttonHover : Colour (0xff15161a));
    g.fillRoundedRectangle (nr, 5.0f);
    g.setColour (Colour (0xff23242a));
    g.drawRoundedRectangle (nr.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (Palette::text);
    g.setFont (Fonts::regular (13.0f));
    g.drawText (proc.getPresetName(), nr, Justification::centred, true);

    // CV id, Mod, Help
    auto cv = area (CV);
    g.setColour (Colour (0xff2a2b32));
    g.drawRoundedRectangle (cv.reduced (0.5f), 4.0f, 1.0f);
    g.setFont (Fonts::mono (10.5f));
    g.setColour (Palette::textDim);
    g.drawText ("CV", cv.withWidth (22.0f).translated (4.0f, 0.0f), Justification::centred, false);
    g.setColour (Palette::osc);
    g.drawText (proc.instanceID, cv.withTrimmedLeft (24.0f), Justification::centredLeft, false);

    const bool modOpen = isModOpen && isModOpen();
    auto mr = area (Mod);
    if (modOpen)
    {
        g.setColour (Colour (0xff2a2415));
        g.fillRoundedRectangle (mr, 4.0f);
    }
    g.setFont (Fonts::regular (12.0f));
    g.setColour (modOpen ? Palette::curve : (hover == Mod ? Palette::text : Palette::textDim));
    g.drawText ("Mod", mr, Justification::centred, false);
    g.setColour (hover == Help ? Palette::text : Palette::textDim);
    g.drawText ("Help", area (Help), Justification::centred, false);
}

void TopBar::mouseMove (const MouseEvent& e)
{
    const int h = itemAt (e.position);
    if (h != hover) { hover = h; repaint(); }
}

void TopBar::mouseExit (const MouseEvent&) { hover = None; repaint(); }

String TopBar::getTooltipFor (Point<float> p) const
{
    switch (itemAt (p))
    {
        case Undo: return "Undo (Cmd/Ctrl+Z)";
        case Redo: return "Redo (Shift+Cmd/Ctrl+Z)";
        case AB:   return "Compare two settings: click to switch between A and B";
        case Copy: return "Copy the current setting to the other slot (A to B or B to A)";
        case Name: return "Presets: factory sounds by category, save and load your own";
        case CV:   return "Instance ID for Virtual CV between ResoOG instances (coming in a later version)";
        case Mod:  return "Modulation panel: all routings, amounts, controllers and functions (key 9)";
        case Help: return "Help, tooltips, window size";
        default:   return {};
    }
}

void TopBar::mouseDown (const MouseEvent& e)
{
    const int it = itemAt (e.position);
    auto* h = proc.history.get();
    if (it >= 0 && it < 5) { if (onTab) onTab (it); return; }
    switch (it)
    {
        case Undo: if (h) h->undo(); break;
        case Redo: if (h) h->redo(); break;
        case AB:   if (h) h->switchTo (1 - h->getActiveSlot()); break;
        case Copy: if (h) h->copyToOther(); break;
        case Prev: stepPreset (-1); break;
        case Next: stepPreset (1); break;
        case Name: showPresetMenu(); break;
        case Mod:  if (onMod) onMod(); break;
        case Help: showHelpMenu(); break;
        default: break;
    }
    repaint();
}

void TopBar::stepPreset (int dir)
{
    const int n = (int) factoryPresets().size();
    proc.loadFactoryPreset (((proc.getCurrentProgram() + dir) % n + n) % n);
    refresh();
}

void TopBar::showPresetMenu()
{
    PopupMenu m;
    StringArray cats;
    for (auto& p : factoryPresets()) cats.addIfNotAlreadyThere (p.category);
    PopupMenu factory;
    for (auto& c : cats)
    {
        PopupMenu sub;
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
            if (c == factoryPresets()[(size_t) i].category)
                sub.addItem (100 + i, factoryPresets()[(size_t) i].name, true, proc.getPresetName() == factoryPresets()[(size_t) i].name);
        factory.addSubMenu (c, sub);
    }
    m.addSubMenu ("Factory", factory);
    m.addSeparator();
    m.addItem (1, "Save preset...");
    m.addItem (2, "Load preset...");
    m.addSeparator();
    m.addItem (3, "Init (reset everything)");

    // description of the current factory preset
    for (auto& p : factoryPresets())
        if (proc.getPresetName() == p.name)
        {
            m.addSeparator();
            m.addSectionHeader (p.description);
        }

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (area (Name).toNearestInt())),
                     [sp = SafePointer<TopBar> (this)] (int r)
    {
        if (sp == nullptr || r == 0) return;
        if (r >= 100) sp->proc.loadFactoryPreset (r - 100);
        else if (r == 1) sp->savePreset();
        else if (r == 2) sp->loadPreset();
        else if (r == 3) sp->proc.loadFactoryPreset (0);
        sp->refresh();
    });
}

static File presetFolder()
{
    auto f = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Gavr").getChildFile ("ResoOG Presets");
    f.createDirectory();
    return f;
}

void TopBar::savePreset()
{
    chooser = std::make_unique<FileChooser> ("Save preset", presetFolder().getChildFile (proc.getPresetName() + ".resoog"), "*.resoog");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                          [sp = SafePointer<TopBar> (this)] (const FileChooser& fc)
    {
        if (sp == nullptr) return;
        auto f = fc.getResult();
        if (f == File()) return;
        sp->proc.setPresetName (f.getFileNameWithoutExtension());
        MemoryBlock mb;
        sp->proc.getStateInformation (mb);
        f.withFileExtension ("resoog").replaceWithData (mb.getData(), mb.getSize());
        sp->refresh();
    });
}

void TopBar::loadPreset()
{
    chooser = std::make_unique<FileChooser> ("Load preset", presetFolder(), "*.resoog");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [sp = SafePointer<TopBar> (this)] (const FileChooser& fc)
    {
        if (sp == nullptr) return;
        auto f = fc.getResult();
        MemoryBlock mb;
        if (f.existsAsFile() && f.loadFileAsData (mb))
        {
            sp->proc.setStateInformation (mb.getData(), (int) mb.getSize());
            sp->proc.setPresetName (f.getFileNameWithoutExtension());
            sp->refresh();
        }
    });
}

void TopBar::showHelpMenu()
{
    PopupMenu m;
    m.addItem (1, "Show tooltips", true, settings.getBool (GlobalSettings::tooltips));
    PopupMenu size;
    const int cur = getScale ? getScale() : 100;
    for (int s : { 70, 80, 90, 100, 115, 130, 150 })
        size.addItem (100 + s, String (s) + " %", true, cur == s);
    m.addSubMenu ("Window size", size);
    m.addSeparator();
    m.addSectionHeader ("Keys: 1-5 pages, 9 modulation panel, Cmd+Z undo");
    m.addSectionHeader ("Knobs: drag, Shift = fine, double-click = type, Cmd-click = default");
    m.addSectionHeader ("Drag the arrow in a CNTRL header onto any knob to modulate it");
    m.addSeparator();
    m.addItem (2, "About ResoOG " + String (JucePlugin_VersionString) + " - Gavr", false);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (area (Help).toNearestInt())),
                     [sp = SafePointer<TopBar> (this)] (int r)
    {
        if (sp == nullptr || r == 0) return;
        if (r == 1) sp->settings.setBool (GlobalSettings::tooltips, ! sp->settings.getBool (GlobalSettings::tooltips));
        else if (r >= 100 && sp->onScale) sp->onScale (r - 100);
    });
}

//==============================================================================
BottomBar::BottomBar (ResoOGProcessor& p) : proc (p) {}

Rectangle<float> BottomBar::area (int item) const
{
    switch (item)
    {
        case Voice: return { 10.0f, 0.0f, 128.0f, (float) height };
        case Glide: return { 142.0f, 0.0f, 128.0f, (float) height };
        case Bend:  return { 272.0f, 0.0f, 80.0f, (float) height };
        case OS:    return { (float) getWidth() - 316.0f, 0.0f, 120.0f, (float) height };
        default:    return {};
    }
}

int BottomBar::itemAt (Point<float> p) const
{
    for (int i = Voice; i <= OS; ++i) if (area (i).contains (p)) return i;
    return None;
}

void BottomBar::refresh()
{
    const float c = proc.cpuLoad.load(), pk = proc.meterPeak.load();
    const auto n = proc.getPresetName();
    if (std::abs (c - cpu) > 0.5f || std::abs (pk - peak) > 0.3f || n != preset)
    {
        cpu = c; peak = pk; preset = n;
        repaint();
    }
}

void BottomBar::paint (Graphics& g)
{
    g.fillAll (Palette::bar);
    g.setColour (Colour (0xff16171b));
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
    g.setFont (Fonts::regular (12.0f));

    const bool duo = proc.getParamReal ("voice_mode") > 0.5f;
    g.setColour (Palette::textDim);
    g.drawText (duo ? "Mono - Duophonic" : "Mono - Layered", area (Voice), Justification::centredLeft, false);
    g.drawText (proc.getParamReal ("glide_legato") > 0.5f ? "Glide: legato" : "Glide: always", area (Glide), Justification::centredLeft, false);
    g.drawText ("PB " + String (CharPointer_UTF8 ("\xc2\xb1")) + String (roundToInt (proc.getParamReal ("pb_range"))), area (Bend), Justification::centredLeft, false);

    // preset pill
    const float w = 240.0f;
    auto pill = Rectangle<float> ((getWidth() - w) * 0.5f, 3.0f, w, (float) height - 6.0f);
    g.setColour (Colour (0xff15161a));
    g.fillRoundedRectangle (pill, 5.0f);
    String cat;
    for (auto& p : factoryPresets()) if (preset == p.name) cat = p.category;
    g.setColour (Palette::curve);
    g.fillEllipse (pill.getX() + 14.0f, pill.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour (Palette::text);
    g.drawText ((cat.isNotEmpty() ? cat + " - " : String()) + preset, pill.withTrimmedLeft (26.0f).withTrimmedRight (10.0f), Justification::centred, true);

    const int os = roundToInt (proc.getParamReal ("oversampling"));
    g.setColour (Palette::textDim);
    g.drawText ("Oversampling " + String (1 << os) + "x", area (OS), Justification::centredRight, false);
    g.setFont (Fonts::mono (11.5f));
    g.setColour (Palette::text);
    g.drawText ("CPU " + String (roundToInt (cpu)) + " %", getWidth() - 186, 0, 74, height, Justification::centredRight, false);
    g.setColour (peak > -0.1f ? Palette::red : Palette::text);
    g.drawText (peak < -90.0f ? String ("-inf dB") : String (peak, 1) + " dB", getWidth() - 106, 0, 74, height, Justification::centredRight, false);
}

void BottomBar::mouseMove (const MouseEvent& e)
{
    switch (itemAt (e.position))
    {
        case Voice: setTooltip ("Layered: both synths play the same note. Duophonic: synth 1 = lowest, synth 2 = highest held note"); break;
        case Glide: setTooltip ("Glide on every note, or only between overlapping (legato) notes"); break;
        case Bend:  setTooltip ("Pitch bend range"); break;
        case OS:    setTooltip ("Oversampling of oscillators and filters: 2x is the sweet spot, 4x for extreme resonance and sync"); break;
        default:    setTooltip ("Output peak level / CPU of this instance"); break;
    }
}

void BottomBar::mouseDown (const MouseEvent& e)
{
    switch (itemAt (e.position))
    {
        case Voice: proc.setParamReal ("voice_mode", proc.getParamReal ("voice_mode") > 0.5f ? 0.0f : 1.0f); break;
        case Glide: proc.setParamReal ("glide_legato", proc.getParamReal ("glide_legato") > 0.5f ? 0.0f : 1.0f); break;
        case Bend:
        {
            PopupMenu m;
            for (int r : { 1, 2, 3, 5, 7, 12, 24 }) m.addItem (r, "+-" + String (r) + " semitones", true, roundToInt (proc.getParamReal ("pb_range")) == r);
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [sp = SafePointer<BottomBar> (this)] (int r)
            { if (sp && r > 0) { sp->proc.setParamReal ("pb_range", (float) r); sp->repaint(); } });
            break;
        }
        case OS:
        {
            PopupMenu m;
            const int cur = roundToInt (proc.getParamReal ("oversampling"));
            m.addItem (1, "1x - lowest CPU, some aliasing on high notes", true, cur == 0);
            m.addItem (2, "2x - recommended", true, cur == 1);
            m.addItem (3, "4x - cleanest sync and resonance, about 2x CPU", true, cur == 2);
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [sp = SafePointer<BottomBar> (this)] (int r)
            { if (sp && r > 0) { sp->proc.setParamReal ("oversampling", (float) (r - 1)); sp->repaint(); } });
            break;
        }
        default: break;
    }
    repaint();
}
