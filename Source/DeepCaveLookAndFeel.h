#pragma once

#include <JuceHeader.h>

class DeepCaveLookAndFeel : public juce::LookAndFeel_V4 {
public:
  DeepCaveLookAndFeel();

  void drawWhiteNote(int note, juce::Graphics &g, juce::Rectangle<float> area,
                     bool isDown, bool isOver, juce::Colour lineColour,
                     juce::Colour textColour);
  void drawBlackNote(int note, juce::Graphics &g, juce::Rectangle<float> area,
                     bool isDown, bool isOver, juce::Colour noteFillColour);
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider &slider) override;

  // Non-override helper for panels
  void drawPanel(juce::Graphics &g, juce::Rectangle<float> area,
                 const juce::String &title);

  void drawLogo(juce::Graphics &g, juce::Rectangle<float> area);
};
