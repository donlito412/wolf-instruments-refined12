#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "VisualizerComponent.h"

//==============================================================================

//==============================================================================
HowlingWolvesAudioProcessorEditor::HowlingWolvesAudioProcessorEditor(
    HowlingWolvesAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      attackAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), ParamIDs::attack, attackSlider)),
      decayAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), ParamIDs::decay, decaySlider)),
      sustainAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), ParamIDs::sustain, sustainSlider)),
      releaseAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), ParamIDs::release, releaseSlider)),
      gainAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), ParamIDs::gain, gainSlider)),
      keyboardComponent(audioProcessor.getKeyboardState(),
                        juce::MidiKeyboardComponent::horizontalKeyboard),
      presetBrowser(audioProcessor.getPresetManager()) {

  // Connect Visualizer to Processor FIFO
  visualizer.setFIFO(&audioProcessor.visualizerFIFO);
  addAndMakeVisible(visualizer);

  // Load Background
  backgroundImage = juce::ImageCache::getFromMemory(
      BinaryData::background_png, BinaryData::background_pngSize);

  // Setup Keyboard
  addAndMakeVisible(keyboardComponent);
  keyboardComponent.setAvailableRange(24, 96);
  keyboardComponent.setKeyWidth(20); // Reduced from 30 to 20 ("Skinnier")
  keyboardComponent.setColour(
      juce::MidiKeyboardComponent::keyDownOverlayColourId,
      juce::Colour::fromString("FF88CCFF")); // Ice Blue key press
  keyboardComponent.setColour(
      juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
      juce::Colour::fromString("FF88CCFF").withAlpha(0.5f));

  // Initialize UI (See implementation plan)
  deepCaveLookAndFeel.setColour(juce::Slider::thumbColourId,
                                juce::Colour::fromString("FFB0B0B0"));

  // Common Slider Setup Helper
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label,
                            const juce::String &name) {
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&deepCaveLookAndFeel);

    addAndMakeVisible(label);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false); // Label above slider
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId,
                    juce::Colours::white.withAlpha(0.8f));
  };

  setupSlider(attackSlider, attackLabel, "ATTACK");
  setupSlider(decaySlider, decayLabel, "DECAY");
  setupSlider(sustainSlider, sustainLabel, "SUSTAIN");
  setupSlider(releaseSlider, releaseLabel, "RELEASE");
  setupSlider(gainSlider, gainLabel, "GAIN");

  // Setup Keyboard Look
  keyboardComponent.setLookAndFeel(&deepCaveLookAndFeel);
  keyboardComponent.setBlackNoteLengthProportion(0.6f);

  // Make plugin resizable
  setResizable(true, true);
  setResizeLimits(600, 400, 1200, 800);
  setSize(800, 600);

  // Top Bar Buttons
  addAndMakeVisible(browseButton);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(settingsButton);

  browseButton.setLookAndFeel(&deepCaveLookAndFeel);
  saveButton.setLookAndFeel(&deepCaveLookAndFeel);
  settingsButton.setLookAndFeel(&deepCaveLookAndFeel);

  // Button Callbacks
  browseButton.onClick = [this] {
    // Toggle Browser
    bool isVisible = presetBrowser.isVisible();
    presetBrowser.setVisible(!isVisible);
    if (!isVisible) {
      presetBrowser.toFront(true);
      presetBrowser.refresh();
    }
  };

  saveButton.onClick = [this] {
    // Quick Save (Demo)
    audioProcessor.getPresetManager().savePreset(
        "New Preset " + juce::Time::getCurrentTime().toString(true, true));
  };

  // Browser (Always on top)
  addChildComponent(presetBrowser);
  presetBrowser.setVisible(false); // Hidden by default
}

HowlingWolvesAudioProcessorEditor::~HowlingWolvesAudioProcessorEditor() {
  visualizer.setFIFO(nullptr);
}

void HowlingWolvesAudioProcessorEditor::paint(juce::Graphics &g) {
  // 1. Background (Cave)
  if (backgroundImage.isValid())
    g.drawImage(backgroundImage, getLocalBounds().toFloat());
  else
    g.fillAll(juce::Colour::fromString("FF101012")); // Fallback

  auto area = getLocalBounds().toFloat();

  // 1. Remove Top Bar and Keyboard FIRST to define safe Main Area
  auto topBarArea = area.removeFromTop(50.0f);
  auto keyboardArea = area.removeFromBottom(50.0f);

  // Now area is strictly the middle space
  // Now area is strictly the middle space
  auto mainArea = area;
  mainArea.reduce(25, 25); // Increased outer padding from 15 to 25

  // 2. Top Bar Background (Dark Strap - More transparent)
  g.setColour(juce::Colour::fromString("FF0A0A0C").withAlpha(0.7f));
  g.fillRect(topBarArea);
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(topBarArea.removeFromBottom(1), 1.0f); // Bottom separator

  // Title & Subtitle (Left/Center)
  auto titleArea = topBarArea.removeFromLeft(250); // Increased width for logo

  // Use new Vector Logo
  deepCaveLookAndFeel.drawLogo(g, titleArea.removeFromTop(30).toFloat());

  g.setColour(juce::Colours::white.withAlpha(0.6f));
  g.setFont(11.0f); // Reduced font size
  g.drawText("Unleash Your Sound", titleArea, juce::Justification::centredTop,
             false);

  // Preset LCD (Centered between Title and Buttons)
  // topBarArea currently starts after the Title (250px)
  // We need to exclude the button area on the right (220px)
  auto centralArea = topBarArea;
  centralArea.removeFromRight(220);

  auto lcdArea = centralArea.withSizeKeepingCentre(180, 30);

  g.setColour(juce::Colour::fromString("FF000000").withAlpha(0.5f));
  g.fillRoundedRectangle(lcdArea, 4.0f);
  g.setColour(juce::Colour::fromString("FF333333"));
  g.drawRoundedRectangle(lcdArea, 4.0f, 1.0f);
  g.setColour(juce::Colours::white);
  g.setFont(13.0f);
  g.drawText("PRESET: Dark Hunter", lcdArea.reduced(8),
             juce::Justification::centredLeft, false);

  // 3. Panels (Floating) - strictly within mainArea
  // 3. Panels (Floating) - strictly within mainArea
  mainArea.reduce(10, 10); // Additional inner padding

  float gap = 20.0f; // Increased gap from 15 to 20
  float panelW = (mainArea.getWidth() - gap) / 2.0f;
  float topRowH = mainArea.getHeight() * 0.45f; // Adjusted proportion
  float bottomRowH = mainArea.getHeight() - topRowH - gap;

  // Row 1
  deepCaveLookAndFeel.drawPanel(
      g, juce::Rectangle<float>(mainArea.getX(), mainArea.getY(), 140, topRowH),
      "SOUND ENGINE"); // Left small
  deepCaveLookAndFeel.drawPanel(
      g,
      juce::Rectangle<float>(mainArea.getX() + 140 + gap, mainArea.getY(),
                             mainArea.getWidth() - (140 + gap), topRowH),
      "MODULATION"); // Wide Mod panel

  // Row 2
  float r2y = mainArea.getY() + topRowH + gap;
  deepCaveLookAndFeel.drawPanel(
      g, juce::Rectangle<float>(mainArea.getX(), r2y, panelW, bottomRowH),
      "FILTER & DRIVE");
  deepCaveLookAndFeel.drawPanel(
      g,
      juce::Rectangle<float>(mainArea.getX() + panelW + gap, r2y, panelW,
                             bottomRowH),
      "OUTPUT");

  // Shadow above keyboard (drawn relative to bottom of valid area)
  auto shadowY = getLocalBounds().getBottom() - 50;
  g.setGradientFill(juce::ColourGradient(
      juce::Colours::black.withAlpha(0.8f), 0, (float)shadowY,
      juce::Colours::transparentBlack, 0, (float)shadowY - 20, false));
  g.fillRect(0, shadowY - 20, getWidth(), 20);
}

void HowlingWolvesAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // 1. Remove Top Bar and Keyboard FIRST to define safe Main Area
  auto topBar = area.removeFromTop(50);
  auto keyboardArea = area.removeFromBottom(50);

  // 2. Position Fixed Components
  // Buttons
  auto buttonArea = topBar.removeFromRight(220).reduced(5);
  browseButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  saveButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  settingsButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));

  // Keyboard
  keyboardComponent.setBounds(keyboardArea);

  // 3. Main Body - Layout Panels within the remaining 'area'
  auto mainArea = area;
  mainArea.reduce(25, 25); // Match paint() outer padding

  float gap = 20.0f; // Match paint() gap
  float topRowH = mainArea.getHeight() * 0.45f;

  // Mod Panel starts after Sound Panel (140px) + gap
  auto modPanel = mainArea.withX(mainArea.getX() + 140 + gap)
                      .withWidth(mainArea.getWidth() - (140 + gap))
                      .withHeight(topRowH)
                      .reduced(10, 10); // padding inside panel

  // Place ADSR in Mod Panel (Distributed Evenly)
  // Dynamic Scaling: Calculate Max knob size fitting in the remaining vertical
  // space Available Height = Panel Height - Top Offset (75) - Bottom Margin
  // (10)
  int availableHeight = (int)modPanel.getHeight() - 85;
  int knobSize =
      juce::jlimit(30, 45, availableHeight); // Reduced Max Size from 55 to 45

  juce::FlexBox flexBox;
  flexBox.flexDirection = juce::FlexBox::Direction::row;
  flexBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
  flexBox.alignContent = juce::FlexBox::AlignContent::center;

  flexBox.items.add(
      juce::FlexItem(attackSlider).withWidth(knobSize).withHeight(knobSize));
  flexBox.items.add(
      juce::FlexItem(decaySlider).withWidth(knobSize).withHeight(knobSize));
  flexBox.items.add(
      juce::FlexItem(sustainSlider).withWidth(knobSize).withHeight(knobSize));
  flexBox.items.add(
      juce::FlexItem(releaseSlider).withWidth(knobSize).withHeight(knobSize));

  // modPanel is float, convert for FlexBox and Account for Title Header (75px)
  auto modContent = modPanel.toNearestInt();
  modContent.removeFromTop(75);
  flexBox.performLayout(modContent);

  // Place Gain in "OUTPUT" Panel (Bottom Right)
  // ... (Gain layout omitted for brevity, assuming it's handled or implicit)

  // (Old block removed)
  // Resume layout for Output Panel
  float bottomRowH =
      mainArea.getHeight() - topRowH - gap; // mainArea is available
  // Re-calculate panelW if needed, or reuse if defined earlier.
  // Wait, panelW was defined in Step 1035 output line 450?
  // No, line 450 defines gap and topRowH. panelW is used in line 500 but not
  // defined in visible block 426-486. Actually panelW IS defined in step 927
  // line 388/397 (in paint) but in resized? Let me define it to be safe.
  // Stack Visualizer (Top) and Gain (Bottom) in Output Panel
  // Re-define panelW if needed (safe scope)
  float panelW = (mainArea.getWidth() - gap) / 2.0f;

  auto outputPanel = mainArea.withY(mainArea.getY() + topRowH + gap)
                         .withX(mainArea.getX() + panelW + gap) // Right side
                         .withWidth(panelW)
                         .withHeight(bottomRowH)
                         .reduced(10);

  auto visualizerSlot =
      outputPanel.removeFromTop(outputPanel.getHeight() * 0.5f);
  // Make flexible but smaller and centered
  // Flexible size, centered logic
  // User requested "Move to left just a little bit" -> (-4, 12)
  visualizer.setBounds(visualizerSlot.reduced(20, 10).translated(-4, 12));

  // Center Gain in remaining lower half
  gainSlider.setBounds(outputPanel.getCentreX() - knobSize / 2,
                       outputPanel.getCentreY() - knobSize / 2, knobSize,
                       knobSize);

  // Overlay: Preset Browser (Full Area minus Top Bar)
  auto browserArea = getLocalBounds();
  browserArea.removeFromTop(50);
  presetBrowser.setBounds(browserArea);
}
