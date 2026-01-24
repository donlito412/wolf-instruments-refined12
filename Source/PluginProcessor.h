#pragma once

#include "EffectsProcessor.h"
#include "FilterProcessor.h"
#include "LFOProcessor.h"
#include "MidiProcessor.h"
#include "PresetManager.h"
#include "SampleManager.h"
#include "SynthEngine.h"
#include <JuceHeader.h>

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

  // Visualizer FIFO
  // Ideally, PluginProcessor polls this, but we need to push to it.
  // Actually, VisualizerComponent has the FIFO. Editor owns
  // VisualizerComponent. So Processor needs to push to Editor? No, bad
  // coupling. Better: Processor has a method `pushToVisualizer` or just exposes
  // a lock-free FIFO that Editor reads. OR: Editor passes its Visualizer's
  // "push" method to Processor as a lambda? Let's use a public AudioBuffer in
  // Processor that Editor reads? No, thread safety.

  // Simplest: Processor owns a `VisualizerFIFO` (like AbstractFifo wrapper) and
  // exposes it. But our VisualizerComponent *already* has the FIFO logic inside
  // it. So we just need to get data to it. Let's add a Safe pointer or use a
  // generic Broadcaster.

  // Let's go with: Processor has a lock-free FIFO. Editor reads from it to feed
  // VisualizerComponent. Actually, I'll just add `pushToVisualizer(buffer)`
  // which calls a hook if set.
  std::function<void(const juce::AudioBuffer<float> &)> audioVisualizerHook;

private:
  //==============================================================================
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  juce::AudioProcessorValueTreeState apvts;

  SynthEngine synthEngine;
  SampleManager sampleManager;
  juce::MidiKeyboardState keyboardState;
  PresetManager presetManager;

  // Filter and LFO
  FilterProcessor filterProcessor;
  LFOProcessor lfoProcessor;
  EffectsProcessor effectsProcessor;
  MidiProcessor midiProcessor;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HowlingWolvesAudioProcessor)
};
