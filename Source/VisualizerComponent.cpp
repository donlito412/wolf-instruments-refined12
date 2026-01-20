#include "VisualizerComponent.h"

VisualizerComponent::VisualizerComponent() {
  startTimerHz(30); // 30 FPS Refresh
  scopeData.fill(0.0f);
}

VisualizerComponent::~VisualizerComponent() { stopTimer(); }

void VisualizerComponent::pushBuffer(const juce::AudioBuffer<float> &buffer) {
  // We don't push from here anymore if we are using the external FIFO directly
  // from Processor. Ideally, this method is deprecated or changed. But if the
  // Processor calls visualizerFIFO.push, we don't need this method strictly
  // speaking, unless we want to keep it as a wrapper? Actually, Processor calls
  // `visualizerFIFO.push`, not `visualizer.pushBuffer`. So this method is
  // likely unused now, but let's keep it safe or empty.
  if (externalFIFO) {
    externalFIFO->push(buffer);
  }
}

void VisualizerComponent::timerCallback() {
  if (!externalFIFO)
    return;

  std::vector<float> readBuffer;
  externalFIFO->pop(readBuffer);

  if (readBuffer.empty())
    return;

  // Update scopeData by shifting and appending
  // Simple optimization: just copy the last N samples if we have enough
  size_t numNewSamples = readBuffer.size();

  if (numNewSamples >= scopeSize) {
    // If we have more new data than the scope size, just copy the latest chunk
    std::copy(readBuffer.end() - scopeSize, readBuffer.end(),
              scopeData.begin());
  } else {
    // Shift existing data left
    std::copy(scopeData.begin() + numNewSamples, scopeData.end(),
              scopeData.begin());
    // Append new data at the end
    std::copy(readBuffer.begin(), readBuffer.end(),
              scopeData.end() - numNewSamples);
  }

  repaint();
}

void VisualizerComponent::paint(juce::Graphics &g) {
  auto area = getLocalBounds();
  g.setColour(juce::Colour::fromString("FF111111")
                  .withAlpha(0.5f)); // Semi-transparent bg
  g.fillRoundedRectangle(area.toFloat(), 5.0f);

  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRoundedRectangle(area.toFloat(), 5.0f, 1.0f); // Border

  g.setColour(juce::Colour::fromString("FF88CCFF")); // Ice Blue Wave

  juce::Path wavePath;
  float w = (float)getWidth();
  float h = (float)getHeight();
  float centerY = h * 0.5f;
  float scaleY = h * 0.45f; // Almost full height

  wavePath.startNewSubPath(0, centerY + scopeData[0] * scaleY);
  float xInc = w / (float)scopeSize;

  for (int i = 1; i < scopeSize; ++i) {
    wavePath.lineTo((float)i * xInc, centerY + scopeData[i] * scaleY);
  }

  g.strokePath(wavePath, juce::PathStrokeType(1.5f));
}
