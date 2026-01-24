#pragma once

#include "EffectsTab.h" // Added
#include "MidiTab.h"
#include "ModernCyberLookAndFeel.h"
#include "ModulateTab.h"
#include "PlayTab.h"
#include "PluginProcessor.h"
#include "SettingsTab.h" // Added
#include <JuceHeader.h>

//==============================================================================
// Placeholder tab components (will be implemented in later phases)
//==============================================================================
// PlayTab class definition removed (now in PlayTab.h)
// ModulateTab class definition removed (now in ModulateTab.h)
// EffectsTab class definition removed (now in EffectsTab.h)
// SettingsTab class definition removed (now in SettingsTab.h)

//==============================================================================
class HowlingWolvesAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  HowlingWolvesAudioProcessorEditor(HowlingWolvesAudioProcessor &);
  ~HowlingWolvesAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // Modern UI components
  ModernCyberLookAndFeel modernLookAndFeel;
  juce::TabbedComponent tabs;
  juce::ComponentBoundsConstrainer constrainer;

  // Keyboard (always visible at bottom)
  juce::MidiKeyboardComponent keyboardComponent;

  // Cave background
  juce::Image backgroundImage;

  // Top bar buttons
  juce::TextButton browseButton{"BROWSE"};
  juce::TextButton saveButton{"SAVE"};
  juce::TextButton settingsButton{"?"};

  // Preset browser overlay
  PresetBrowser presetBrowser;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      HowlingWolvesAudioProcessorEditor)
};
