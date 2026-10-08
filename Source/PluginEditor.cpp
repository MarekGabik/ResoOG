#include "PluginEditor.h"

using namespace juce;

//==============================================================================
void Stage::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Palette::stageTop, 0.0f, 0.0f, Palette::stageBottom, 0.0f, r.getHeight(), false));
    g.fillRect (r);

    // fine grain texture, drawn once
    if (texture.isNull())
    {
        texture = Image (Image::ARGB, 160, 160, true);
        Random rnd (7);
        for (int y = 0; y < 160; ++y)
            for (int x = 0; x < 160; ++x)
                texture.setPixelAt (x, y, Colours::white.withAlpha (rnd.nextFloat() * 0.035f));
    }
    g.setTiledImageFill (texture, 0, 0, 1.0f);
    g.fillRect (r);

    // walnut cheeks
    auto cheek = [&g] (Rectangle<float> c, bool left)
    {
        g.setGradientFill (ColourGradient (Colour (0xff3b2414), c.getX(), 0.0f, Colour (0xff3a2212), c.getRight(), 0.0f, false));
        ColourGradient wood (Colour (0xff3b2414), c.getX(), 0.0f, Colour (0xff3a2212), c.getRight(), 0.0f, false);
        wood.addColour (0.4, Colour (0xff7a4b27));
        wood.addColour (0.7, Colour (0xff5a3519));
        g.setGradientFill (wood);
        g.fillRect (c);
        g.setColour (Colours::black.withAlpha (0.12f));
        for (float x = c.getX() + 2.0f; x < c.getRight(); x += 3.0f)
            g.drawVerticalLine ((int) x, c.getY(), c.getBottom());
        g.setColour (Colours::black);
        g.drawVerticalLine (left ? (int) c.getRight() - 1 : (int) c.getX(), c.getY(), c.getBottom());
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillRect (left ? c.getRight() : c.getX() - 3.0f, c.getY(), 3.0f, c.getHeight());
    };
    cheek ({ 0.0f, 0.0f, 14.0f, r.getHeight() }, true);
    cheek ({ r.getWidth() - 14.0f, 0.0f, 14.0f, r.getHeight() }, false);

    // name plate
    const float y = 18.0f + (float) Page::height + 12.0f;
    g.setFont (Fonts::bold (32.0f).withExtraKerningFactor (-0.03f));
    g.setColour (Palette::text);
    g.drawText ("Reso", 34, (int) y, 90, 38, Justification::centredLeft, false);
    const int resoW = GlyphArrangement::getStringWidthInt (Fonts::bold (32.0f).withExtraKerningFactor (-0.03f), "Reso");
    g.setColour (Palette::curve);
    g.drawText ("OG", 34 + resoW, (int) y, 60, 38, Justification::centredLeft, false);
    const int ogW = GlyphArrangement::getStringWidthInt (Fonts::bold (32.0f).withExtraKerningFactor (-0.03f), "OG");
    int x = 34 + resoW + ogW + 14;
    g.setColour (Palette::textDim);
    g.setFont (Fonts::bold (11.5f).italicised().withExtraKerningFactor (0.22f));
    g.drawText ("DUAL-LAYER BASS SYNTHESIZER", x, (int) y + 14, 300, 16, Justification::centredLeft, false);
    x += 300;
    if (badge.isNotEmpty())
    {
        auto b = Rectangle<float> ((float) x, y + 12.0f, (float) GlyphArrangement::getStringWidthInt (Fonts::mono (11.0f), badge) + 14.0f, 19.0f);
        g.setColour (Palette::curve.withAlpha (0.4f));
        g.drawRoundedRectangle (b, 4.0f, 1.0f);
        g.setColour (Palette::curve);
        g.setFont (Fonts::mono (11.0f));
        g.drawText (badge, b, Justification::centred, false);
    }
    g.setColour (Colour (0xff8a8b93));
    g.setFont (Fonts::bold (20.0f).withExtraKerningFactor (0.3f));
    g.drawText ("GAVR", getWidth() - 140, (int) y + 4, 106, 30, Justification::centredRight, false);
}

//==============================================================================
ResoOGEditor::ResoOGEditor (ResoOGProcessor& p)
    : AudioProcessorEditor (p), proc (p), top (p, *settings), keyboard (p), bottom (p), drawer (p)
{
    setLookAndFeel (&lnf);
    proc.editorOpen = true;

    addAndMakeVisible (content);
    content.addAndMakeVisible (top);
    content.addAndMakeVisible (stage);
    content.addAndMakeVisible (keyboard);
    content.addAndMakeVisible (bottom);
    content.addChildComponent (drawer);

    pages.add (new SynthPage (p, 0));
    pages.add (new CntrlPage (p, 0));
    pages.add (new SynthPage (p, 1));
    pages.add (new CntrlPage (p, 1));
    pages.add (new OutputPage (p));
    for (auto* pg : pages)
    {
        stage.addChildComponent (pg);
        pg->setModEditCallback ([this] (int slot) { setModDrawer (true, slot); });
    }

    top.onTab = [this] (int i) { showPage (i); };
    top.onMod = [this] { setModDrawer (! drawer.isVisible()); };
    top.isModOpen = [this] { return drawer.isVisible(); };
    top.onScale = [this] (int s) { setScalePercent (s); };
    top.getScale = [this] { return roundToInt (scale * 100.0f); };
    drawer.onClose = [this] { setModDrawer (false); };

    settings->addChangeListener (this);
    updateTooltips();

    scale = jlimit (0.5f, 2.0f, (float) (int) proc.uiState.getProperty ("scale", settings->getInt (GlobalSettings::scaling, 100)) / 100.0f);
    constrainer.setFixedAspectRatio ((double) logicalWidth / logicalHeight);
    constrainer.setSizeLimits (roundToInt (logicalWidth * 0.5f), roundToInt (logicalHeight * 0.5f), logicalWidth * 2, logicalHeight * 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (roundToInt (logicalWidth * scale), roundToInt (logicalHeight * scale));

    showPage ((int) proc.uiState.getProperty ("page", 0));
    top.refresh();
    bottom.refresh();
    setWantsKeyboardFocus (true);
    startTimerHz (30);
}

ResoOGEditor::~ResoOGEditor()
{
    stopTimer();
    proc.editorOpen = false;
    settings->removeChangeListener (this);
    tooltips.reset();
    setLookAndFeel (nullptr);
}

void ResoOGEditor::paint (Graphics& g)
{
    g.fillAll (Palette::bgBottom);
}

void ResoOGEditor::resized()
{
    scale = (float) getWidth() / (float) logicalWidth;
    content.setTransform (AffineTransform::scale (scale));
    content.setBounds (0, 0, logicalWidth, logicalHeight);
    top.setBounds (0, 0, logicalWidth, TopBar::height);
    stage.setBounds (0, TopBar::height, logicalWidth, Stage::height);
    for (auto* pg : pages) pg->setBounds (32, 18, Page::width, Page::height);
    keyboard.setBounds (0, TopBar::height + Stage::height, logicalWidth, logicalHeight - TopBar::height - Stage::height - BottomBar::height);
    bottom.setBounds (0, logicalHeight - BottomBar::height, logicalWidth, BottomBar::height);
    drawer.setBounds (0, TopBar::height, ModDrawer::width, logicalHeight - TopBar::height);
    proc.uiState.setProperty ("scale", roundToInt (scale * 100.0f), nullptr);
}

void ResoOGEditor::setScalePercent (int percent)
{
    settings->setInt (GlobalSettings::scaling, percent);
    setSize (roundToInt (logicalWidth * percent / 100.0f), roundToInt (logicalHeight * percent / 100.0f));
}

void ResoOGEditor::showPage (int index)
{
    page = jlimit (0, 4, index);
    for (int i = 0; i < pages.size(); ++i) pages[i]->setVisible (i == page);
    top.setPage (page);
    stage.setLayerBadge (page < 2 ? "LAYER 1" : (page < 4 ? "LAYER 2" : "MASTER"));
    proc.uiState.setProperty ("page", page, nullptr);
    pages[page]->refreshLive();
}

void ResoOGEditor::setModDrawer (bool open, int slot)
{
    drawer.setVisible (open);
    if (open)
    {
        drawer.refresh();
        if (slot >= 0) drawer.select (slot);
        drawer.toFront (false);
    }
    top.repaint();
}

bool ResoOGEditor::keyPressed (const KeyPress& key)
{
    const auto c = key.getTextCharacter();
    if (c >= '1' && c <= '5') { showPage (c - '1'); return true; }
    if (c == '9' || c == '0') { setModDrawer (! drawer.isVisible()); return true; }
    if (key == KeyPress ('z', ModifierKeys::commandModifier, 0)) { if (proc.history) proc.history->undo(); return true; }
    if (key == KeyPress ('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0)) { if (proc.history) proc.history->redo(); return true; }
    if (key == KeyPress::escapeKey && drawer.isVisible()) { setModDrawer (false); return true; }
    return false;
}

void ResoOGEditor::timerCallback()
{
    pages[page]->refreshLive();
    keyboard.refresh();
    if ((++tick % 3) == 0)
    {
        top.refresh();
        bottom.refresh();
        if (drawer.isVisible()) drawer.refresh();
    }
}

void ResoOGEditor::changeListenerCallback (ChangeBroadcaster*)
{
    updateTooltips();
}

void ResoOGEditor::updateTooltips()
{
    const bool on = settings->getBool (GlobalSettings::tooltips);
    if (on && tooltips == nullptr)
        tooltips = std::make_unique<TooltipWindow> (this, 700);
    else if (! on)
        tooltips.reset();
}
