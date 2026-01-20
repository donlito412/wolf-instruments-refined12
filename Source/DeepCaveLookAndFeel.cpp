#include "DeepCaveLookAndFeel.h"

DeepCaveLookAndFeel::DeepCaveLookAndFeel() {}

void DeepCaveLookAndFeel::drawWhiteNote(int note, juce::Graphics &g,
                                        juce::Rectangle<float> area,
                                        bool isDown, bool isOver,
                                        juce::Colour lineColour,
                                        juce::Colour textColour) {
  auto c = juce::Colour::fromString("FF2A2A30"); // Dark Slate/Charcoal

  if (isDown)
    c = juce::Colour::fromString("FF88CCFF"); // Ice Blue Glow

  // Gradient for depth
  g.setGradientFill(juce::ColourGradient(c.brighter(0.1f), area.getTopLeft(),
                                         c.darker(0.1f), area.getBottomLeft(),
                                         false));
  g.fillRect(area);

  // Silver Edge
  g.setColour(juce::Colour::fromString("FF888890"));
  g.drawRect(area, 1.0f);

  // Bottom shadow simulation
  g.setGradientFill(juce::ColourGradient(
      juce::Colours::black.withAlpha(0.0f),
      area.getBottomLeft().translated(0, -10),
      juce::Colours::black.withAlpha(0.5f), area.getBottomLeft(), false));
  g.fillRect(area.removeFromBottom(10));
}

void DeepCaveLookAndFeel::drawBlackNote(int note, juce::Graphics &g,
                                        juce::Rectangle<float> area,
                                        bool isDown, bool isOver,
                                        juce::Colour noteFillColour) {
  auto c = juce::Colour::fromString("FF101010"); // Near black matte

  if (isDown)
    c = juce::Colour::fromString("FF4477AA"); // Darker Ice Blue

  g.setColour(c);
  g.fillRect(area);

  // Silver Edge
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(area, 1.0f);

  g.setGradientFill(juce::ColourGradient(
      juce::Colours::white.withAlpha(0.1f), area.getTopLeft(),
      juce::Colours::transparentWhite, area.getBottomLeft(), false));
  g.fillRect(area);
}

void DeepCaveLookAndFeel::drawRotarySlider(
    juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {
  auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
  auto centreX = (float)x + (float)width * 0.5f;
  auto centreY = (float)y + (float)height * 0.5f;
  auto rx = centreX - radius;
  auto ry = centreY - radius;
  auto rw = radius * 2.0f;
  auto angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // Brushed Silver Face (Radial Gradient)
  juce::ColourGradient gradient(juce::Colour::fromString("FFCCCCCC"), centreX,
                                centreY, juce::Colour::fromString("FF333333"),
                                rx, ry, true);
  gradient.addColour(0.4, juce::Colour::fromString("FF888888"));
  gradient.addColour(0.7, juce::Colour::fromString("FFAAAAAA"));

  g.setGradientFill(gradient);
  g.fillEllipse(rx, ry, rw, rw);

  // Metallic Rim
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawEllipse(rx, ry, rw, rw, 2.0f);

  // Indicator (Ice Blue)
  juce::Path p;
  auto pointerLength = radius * 0.7f;
  auto pointerThickness = 3.0f;
  p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness,
                 pointerLength);
  p.applyTransform(
      juce::AffineTransform::rotation(angle).translated(centreX, centreY));

  g.setColour(juce::Colour::fromString("FF88CCFF")); // Ice Blue
  g.fillPath(p);

  // Glow effect for indicator
  g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.6f));
  g.strokePath(p, juce::PathStrokeType(2.0f));
}

void DeepCaveLookAndFeel::drawButtonBackground(
    juce::Graphics &g, juce::Button &button,
    const juce::Colour &backgroundColour, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
  auto area = button.getLocalBounds().toFloat();

  // 1. Panel Body (Glass)
  // Brighter on hover (0.5f), standard (0.3f)
  float alpha = shouldDrawButtonAsHighlighted ? 0.5f : 0.3f;
  g.setColour(juce::Colours::black.withAlpha(alpha));
  g.fillRect(area);

  // Active State (Ice Blue Tint)
  if (shouldDrawButtonAsDown) {
    g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.2f));
    g.fillRect(area);
  }

  // 2. Metallic Edge
  // Active = Ice Blue Border, Normal = Metallic
  g.setColour(shouldDrawButtonAsDown ? juce::Colour::fromString("FF88CCFF")
                                     : juce::Colour::fromString("FF666670"));
  g.drawRect(area, 1.0f);

  // 3. Corner Screws (Industrial Feel)
  float screwSize = 3.0f;
  g.setColour(juce::Colour::fromString("FFCCCCCC")); // Silver screws
  // Inset screws slightly
  g.fillEllipse(area.getX() + 3, area.getY() + 3, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 6, area.getY() + 3, screwSize, screwSize);
  g.fillEllipse(area.getX() + 3, area.getBottom() - 6, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 6, area.getBottom() - 6, screwSize,
                screwSize);
}

void DeepCaveLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                           int width, int height,
                                           float sliderPos, float minSliderPos,
                                           float maxSliderPos,
                                           const juce::Slider::SliderStyle,
                                           juce::Slider &slider) {
  auto trackWidth = 4.0f;
  juce::Point<float> startPoint((float)x + (float)width * 0.5f, (float)height);
  juce::Point<float> endPoint((float)x + (float)width * 0.5f, (float)y);

  // Rail
  juce::Path track;
  track.startNewSubPath(startPoint);
  track.lineTo(endPoint);
  g.setColour(juce::Colour::fromString("FF222222"));
  g.strokePath(track, juce::PathStrokeType(trackWidth));

  // Thumb (Fader Cap)
  auto thumbW = 30.0f;
  auto thumbH = 15.0f;
  auto centreY = sliderPos;
  // Limit centreY within bounds (simplified)

  juce::Rectangle<float> thumb(0, 0, thumbW, thumbH);
  thumb.setCentre(startPoint.x, sliderPos);

  // Metallic Cap
  g.setGradientFill(juce::ColourGradient(
      juce::Colour::fromString("FFEEEEEE"), thumb.getTopLeft(),
      juce::Colour::fromString("FF555555"), thumb.getBottomLeft(), false));
  g.fillRoundedRectangle(thumb, 2.0f);

  // Center line on cap
  g.setColour(juce::Colour::fromString("FF333333"));
  g.drawLine(thumb.getCentreX(), thumb.getY(), thumb.getCentreX(),
             thumb.getBottom(), 1.0f);
}

void DeepCaveLookAndFeel::drawPanel(juce::Graphics &g,
                                    juce::Rectangle<float> area,
                                    const juce::String &title) {
  // 1. Background (Dark Glass)
  g.setColour(juce::Colour::fromString("FF0D0D10").withAlpha(0.7f));
  g.fillRoundedRectangle(area, 6.0f);

  // 2. Metallic Border
  g.setColour(juce::Colour::fromString("FF666670")); // Gunmetal
  g.drawRoundedRectangle(area, 6.0f, 1.5f);

  // 3. Header Bar
  auto headerArea = area.removeFromTop(20.0f); // Small header
  juce::Path headerPath;
  headerPath.addRoundedRectangle(headerArea.getX(), headerArea.getY(),
                                 headerArea.getWidth(), headerArea.getHeight(),
                                 6.0f, 6.0f, // Top corners
                                 false, false, false, false);

  g.setGradientFill(juce::ColourGradient(
      juce::Colour::fromString("FF333338"), headerArea.getTopLeft(),
      juce::Colour::fromString("FF1A1A1D"), headerArea.getBottomLeft(), false));
  // Manually fill top with rounded corners using clipping or path?
  // Easier: just fillRounded then rectify the bottom
  g.setColour(juce::Colour::fromString("FF333338"));
  g.fillRoundedRectangle(headerArea.getX(), headerArea.getY(),
                         headerArea.getWidth(), headerArea.getHeight(), 6.0f);
  // Flatten bottom corners of header?
  // Actually, visual polish: just draw a line separator
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(headerArea.removeFromBottom(1.0f));

  // 4. Panel Title
  g.setColour(juce::Colours::white.withAlpha(0.9f));
  g.setFont(juce::Font(12.0f, juce::Font::bold));
  g.drawText(title, headerArea.reduced(5, 0), juce::Justification::centredLeft,
             false);

  // 5. Screws
  float screwSize = 4.0f;
  g.setColour(juce::Colour::fromString("FF999999"));
  g.fillEllipse(area.getX() + 4, area.getY() + 4, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 8, area.getY() + 4, screwSize, screwSize);
  g.fillEllipse(area.getX() + 4, area.getBottom() - 8, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 8, area.getBottom() - 8, screwSize,
                screwSize);
}

void DeepCaveLookAndFeel::drawLogo(juce::Graphics &g,
                                   juce::Rectangle<float> area) {
  // 1. Create Text Path Directly
  juce::Font logoFont(24.0f, juce::Font::bold);
  logoFont.setExtraKerningFactor(0.15f); // Cinematic spacing

  juce::Path textPath;
  juce::GlyphArrangement glyphs;
  glyphs.addLineOfText(logoFont, "HOWLING WOLVES", 0, 0);
  glyphs.createPath(textPath);

  // Center the path in the area
  auto pathBounds = textPath.getBounds();
  auto offset = area.getCentre() - pathBounds.getCentre();
  textPath.applyTransform(
      juce::AffineTransform::translation(offset.x, offset.y));

  // 2. Glow / Backlight (Ice Blue)
  g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.3f));
  for (int i = 0; i < 3; ++i) {
    g.strokePath(textPath, juce::PathStrokeType(6.0f - i * 1.5f));
  }

  // 3. Metallic Fill (Gradient)
  // Recalculate bounds after transform
  pathBounds = textPath.getBounds();
  juce::ColourGradient metalGradient(
      juce::Colour::fromString("FFEEEEEE"), 0, pathBounds.getY(),
      juce::Colour::fromString("FF444444"), 0, pathBounds.getBottom(), false);

  g.setGradientFill(metalGradient);
  g.fillPath(textPath);

  // 4. White Rim for sharpness
  g.setColour(juce::Colours::white.withAlpha(0.4f));
  g.strokePath(textPath, juce::PathStrokeType(1.0f));
}
