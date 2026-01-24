#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
HowlingWolvesAudioProcessorEditor::HowlingWolvesAudioProcessorEditor(
    HowlingWolvesAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      tabs(juce::TabbedButtonBar::TabsAtTop),
      keyboardComponent(audioProcessor.getKeyboardState(),
                        juce::MidiKeyboardComponent::horizontalKeyboard),
      presetBrowser(audioProcessor.getPresetManager()) {

  // Set LookAndFeel
  setLookAndFeel(&modernLookAndFeel);
  tabs.setLookAndFeel(&modernLookAndFeel);

  // Load cave background
  backgroundImage = juce::ImageCache::getFromMemory(
      BinaryData::howling_wolves_cave_bg_1768783846310_png,
      BinaryData::howling_wolves_cave_bg_1768783846310_pngSize);

  // Set up size constraints
  constrainer.setMinimumSize(700, 500);
  constrainer.setMaximumSize(1200, 800);
  setResizable(true, true);
  setResizeLimits(700, 500, 1200, 800);

  // Set initial size
  setSize(800, 545);

  // Create and add tabs
  tabs.addTab("PLAY", WolfColors::PANEL_DARK, new PlayTab(audioProcessor),
              true); // Pass audioProcessor
  tabs.addTab("MODULATE", WolfColors::PANEL_DARK,
              new ModulateTab(audioProcessor), true); // Pass audioProcessor
  tabs.addTab("PERFORM", WolfColors::PANEL_DARK, new MidiTab(audioProcessor),
              true);
  tabs.addTab("EFFECTS", WolfColors::PANEL_DARK, new EffectsTab(audioProcessor),
              true);
  tabs.addTab("SETTINGS", WolfColors::PANEL_DARK,
              new SettingsTab(audioProcessor), true);
  tabs.setCurrentTabIndex(0);
  addAndMakeVisible(tabs);

  // Top bar buttons
  browseButton.setButtonText("Select a Preset");
  browseButton.onClick = [this] {
    presetBrowser.setVisible(!presetBrowser.isVisible());
    presetBrowser.toFront(true);
    resized(); // Force layout update to position the browser
  };
  addAndMakeVisible(browseButton);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(settingsButton);

  // Keyboard
  addAndMakeVisible(keyboardComponent);

  // Preset browser overlay
  addChildComponent(presetBrowser);
  presetBrowser.setVisible(false);

  // Handle preset selection
  presetBrowser.onPresetSelected = [this](const juce::String &presetName) {
    browseButton.setButtonText(presetName);
    presetBrowser.setVisible(false);
  };
}

HowlingWolvesAudioProcessorEditor::~HowlingWolvesAudioProcessorEditor() {
  setLookAndFeel(nullptr);
  tabs.setLookAndFeel(nullptr);
}

//==============================================================================
void HowlingWolvesAudioProcessorEditor::paint(juce::Graphics &g) {
  // Draw cave background
  g.drawImage(backgroundImage, getLocalBounds().toFloat(),
              juce::RectanglePlacement::fillDestination);

  // Semi-transparent overlay for better contrast
  g.setColour(juce::Colour(0x40000000)); // 25% black
  g.fillAll();
}

void HowlingWolvesAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // Top bar (35px)
  auto topBar = area.removeFromTop(35);

  // Top bar buttons (right side)
  auto buttonArea = topBar.removeFromRight(300).reduced(5);
  browseButton.setBounds(buttonArea.removeFromLeft(150).reduced(2));
  saveButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  settingsButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));

  // Browser Overlay Position
  if (presetBrowser.isVisible()) {
    presetBrowser.setBounds(browseButton.getX(), browseButton.getBottom() + 5,
                            220, 350);
  }

  // Keyboard (bottom, 80px, stretches full width)
  auto keyboardArea = area.removeFromBottom(80);
  keyboardComponent.setBounds(keyboardArea);

  // Calculate key width for full-width stretching
  int totalKeys = 72; // 6 octaves
  float keyWidth = keyboardArea.getWidth() / (float)(totalKeys * 0.7f);
  keyboardComponent.setKeyWidth(keyWidth);

  // Tabs (remaining space)
  tabs.setBounds(area);
}
