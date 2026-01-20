#pragma once

#include "VisualizerFIFO.h"
#include <JuceHeader.h>
#include <array>
#include <vector>

class VisualizerComponent : public juce::Component, public juce::Timer {
public:
  VisualizerComponent();
  ~VisualizerComponent() override;

  void pushBuffer(const juce::AudioBuffer<float> &buffer);
  void paint(juce::Graphics &g) override;
  void timerCallback() override;

  void setFIFO(VisualizerFIFO *fifo) { externalFIFO = fifo; }

private:
  static const int fftOrder = 11;
  static const int fftSize = 1 << fftOrder;
  static const int scopeSize = 512;

  VisualizerFIFO *externalFIFO = nullptr;
  std::array<float, scopeSize> scopeData;

  // Logic:
  // Audio Thread pushes to FIFO.
  // Timer pops from FIFO to update scopeData.
};
