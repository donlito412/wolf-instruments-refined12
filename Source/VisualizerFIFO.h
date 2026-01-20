#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// Thread-Safe Visualizer FIFO (Josh Hodge Style)
struct VisualizerFIFO {
  static const int queueSize = 4096;
  std::array<float, queueSize> buffer;
  juce::AbstractFifo fifo{queueSize};

  void push(const juce::AudioBuffer<float> &source) {
    if (source.getNumChannels() > 0) {
      auto *channelData = source.getReadPointer(0);
      int numSamples = source.getNumSamples();
      int start1, size1, start2, size2;
      fifo.prepareToWrite(numSamples, start1, size1, start2, size2);
      if (size1 > 0)
        std::copy(channelData, channelData + size1, buffer.data() + start1);
      if (size2 > 0)
        std::copy(channelData + size1, channelData + size1 + size2,
                  buffer.data() + start2);
      fifo.finishedWrite(size1 + size2);
    }
  }

  void pop(std::vector<float> &destination) {
    int numSamples = fifo.getNumReady();
    if (numSamples > 0) {
      std::vector<float> temp(numSamples);
      int start1, size1, start2, size2;
      fifo.prepareToRead(numSamples, start1, size1, start2, size2);
      if (size1 > 0)
        std::copy(buffer.data() + start1, buffer.data() + start1 + size1,
                  temp.data());
      if (size2 > 0)
        std::copy(buffer.data() + start2, buffer.data() + start2 + size2,
                  temp.data() + size1);
      fifo.finishedRead(size1 + size2);
      destination.insert(destination.end(), temp.begin(), temp.end());
    }
  }
};
