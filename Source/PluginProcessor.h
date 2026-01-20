#pragma once

#include "PresetManager.h"
#include "SampleManager.h"
#include "SynthEngine.h"
#include "VisualizerFIFO.h"
#include <JuceHeader.h>

namespace ParamIDs {
static const juce::String attack = "attack";
static const juce::String decay = "decay";
static const juce::String sustain = "sustain";
static const juce::String release = "release";
static const juce::String gain = "gain";
// Future expansion for Delay/Reverb
// static const juce::String delayTime = "delayTime";
// ...
} // namespace ParamIDs

class HowlingWolvesAudioProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  HowlingWolvesAudioProcessor();
  ~HowlingWolvesAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }
  SynthEngine &getSynth() { return synthEngine; }
  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }
  PresetManager &getPresetManager() { return presetManager; }

  // Thread-Safe Visualizer FIFO (Josh Hodge Style)
  VisualizerFIFO visualizerFIFO;

private:
  //==============================================================================
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  juce::AudioProcessorValueTreeState apvts;

  SynthEngine synthEngine;
  SampleManager sampleManager;
  juce::MidiKeyboardState keyboardState;
  PresetManager presetManager;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HowlingWolvesAudioProcessor)
};
