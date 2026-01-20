#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A sound that holds the sample data.
    We inherit from SynthesiserSound directly to have full control,
    although we mimic the API of SamplerSound where possible.
*/
class HowlingSound : public juce::SynthesiserSound {
public:
  HowlingSound(const juce::String &name, juce::AudioBuffer<float> &data,
               double sourceSampleRate, const juce::BigInteger &midiNotes,
               int midiNoteForNormalPitch, double attackTimeSecs,
               double releaseTimeSecs, double maxSampleLengthSeconds);

  bool appliesToNote(int midiNoteNumber) override;
  bool appliesToChannel(int midiChannel) override;

  juce::AudioBuffer<float> &getAudioData() { return data; }
  double getSourceSampleRate() const { return sourceSampleRate; }
  int getMidiNoteForNormalPitch() const { return midiNoteForNormalPitch; }

  // Looping support
  void setLooping(bool loop) { shouldLoop = loop; }
  bool isLooping() const { return shouldLoop; }

  // ADSR params (stored here for simplicity, or could be in Voice)
  double attack = 0.1;
  double release = 0.1;

private:
  juce::String name;
  juce::AudioBuffer<float> data;
  double sourceSampleRate;
  juce::BigInteger midiNotes;
  int midiNoteForNormalPitch;
  bool shouldLoop = false;

  JUCE_LEAK_DETECTOR(HowlingSound)
};

//==============================================================================
/**
    A voice that plays back the HowlingSound.
    Supports pitch shifting and looping.
*/
class HowlingVoice : public juce::SynthesiserVoice {
public:
  HowlingVoice();

  bool canPlaySound(juce::SynthesiserSound *sound) override;

  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;

  void updateADSR(const juce::ADSR::Parameters &params);
  void updateFilter(float cutoff, float resonance);
  void updateLFO(float rate, float depth);
  void prepare(double sampleRate, int samplesPerBlock);

  void pitchWheelMoved(int newPitchWheelValue) override {}
  void controllerMoved(int controllerNumber, int newControllerValue) override {}

  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

private:
  double getPitchRatio(int midiNoteNumber) const;

  double currentSampleRate = 44100.0;
  double sourceSamplePosition = 0.0;
  double pitchRatio = 0.0;
  float level = 0.0f;
  float tailOff = 0.0f;

  bool isLooping = false;

  // Simple envelope for start/stop (de-clicking)
  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;

  // DSP
  juce::dsp::StateVariableTPTFilter<float> filter;
  juce::dsp::Oscillator<float> lfo;
  float lfoDepth = 0.0f;

  JUCE_LEAK_DETECTOR(HowlingVoice)
};

//==============================================================================
/**
    The main synth engine.
*/
class SynthEngine : public juce::Synthesiser {
public:
  SynthEngine();

  void initialize();
  void prepare(double sampleRate, int samplesPerBlock);
  void updateParams(float attack, float decay, float sustain, float release,
                    float cutoff, float resonance, float lfoRate,
                    float lfoDepth);
};
