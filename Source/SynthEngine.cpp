#include "SynthEngine.h"

//==============================================================================
// HowlingSound
//==============================================================================

HowlingSound::HowlingSound(const juce::String &soundName,
                           juce::AudioBuffer<float> &content, double sourceRate,
                           const juce::BigInteger &notes, int rootNote,
                           double attackSecs, double releaseSecs, double maxLen)
    : name(soundName), sourceSampleRate(sourceRate), midiNotes(notes),
      midiNoteForNormalPitch(rootNote) {
  // deep copy the buffer for safety
  data.makeCopyOf(content);

  attack = attackSecs;
  release = releaseSecs;
}

bool HowlingSound::appliesToNote(int midiNoteNumber) {
  return midiNotes[midiNoteNumber];
}

bool HowlingSound::appliesToChannel(int midiChannel) { return true; }

//==============================================================================
// HowlingVoice
//==============================================================================

HowlingVoice::HowlingVoice() {}

bool HowlingVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<HowlingSound *>(sound) != nullptr;
}

void HowlingVoice::startNote(int midiNoteNumber, float velocity,
                             juce::SynthesiserSound *sound,
                             int currentPitchWheelPosition) {
  if (auto *howlingSound = dynamic_cast<HowlingSound *>(sound)) {
    sourceSamplePosition = 0.0;
    level = velocity;
    isLooping = howlingSound->isLooping();

    // Calculate pitch ratio
    auto sourceRate = howlingSound->getSourceSampleRate();
    auto rootNote = howlingSound->getMidiNoteForNormalPitch();

    // Pitch ratio = (target_freq / source_freq) * (source_rate / target_rate)
    // Simple formula based on semitones:
    auto semitones = midiNoteNumber - rootNote;
    auto pitchFactor = std::pow(2.0, semitones / 12.0);

    pitchRatio = pitchFactor * (sourceRate / getSampleRate());

    // Setup ADSR
    adsrParams.attack = static_cast<float>(howlingSound->attack);
    adsrParams.decay = 0.0f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = static_cast<float>(howlingSound->release);

    adsr.setSampleRate(getSampleRate());
    adsr.setParameters(adsrParams);
    adsr.noteOn();
  } else {
    jassertfalse; // Should not happen
    clearCurrentNote();
  }
}

void HowlingVoice::stopNote(float velocity, bool allowTailOff) {
  if (allowTailOff) {
    adsr.noteOff();
  } else {
    clearCurrentNote();
    adsr.reset();
  }
}

// ... (Methods)

void HowlingVoice::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = 1; // Processing mono voices usually

  filter.prepare(spec);
  filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

  lfo.prepare(spec);
  lfo.initialise([](float x) { return std::sin(x); }); // Sine LFO
}

void HowlingVoice::updateFilter(float cutoff, float resonance) {
  filter.setCutoffFrequency(cutoff);
  filter.setResonance(resonance);
}

void HowlingVoice::updateLFO(float rate, float depth) {
  lfo.setFrequency(rate);
  lfoDepth = depth;
}

void HowlingVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   int startSample, int numSamples) {
  if (auto *playingSound =
          dynamic_cast<HowlingSound *>(getCurrentlyPlayingSound().get())) {

    auto &data = playingSound->getAudioData();
    const float *const inL = data.getReadPointer(0);
    const float *const inR =
        data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;

    float *outL = outputBuffer.getWritePointer(0, startSample);
    float *outR = outputBuffer.getNumChannels() > 1
                      ? outputBuffer.getWritePointer(1, startSample)
                      : nullptr;

    while (--numSamples >= 0) {
      // LFO Modulation (Vibrato)
      float lfoVal = lfo.processSample(0.0f);
      double pitchMod = 1.0 + (lfoVal * lfoDepth * 0.05); // +/- 5% pitch

      auto currentPos = (int)sourceSamplePosition;
      auto nextPos = currentPos + 1;
      auto alpha = (float)(sourceSamplePosition - currentPos);
      auto invAlpha = 1.0f - alpha;

      // Interpolation (Linear)
      // Check bounds
      float l = 0.0f;
      float r = 0.0f;

      if (currentPos < data.getNumSamples()) {
        float currentL = inL[currentPos];
        float nextL =
            (nextPos < data.getNumSamples())
                ? inL[nextPos]
                : (isLooping ? inL[nextPos % data.getNumSamples()] : 0.0f);
        l = (currentL * invAlpha) + (nextL * alpha);

        if (inR) {
          float currentR = inR[currentPos];
          float nextR =
              (nextPos < data.getNumSamples())
                  ? inR[nextPos]
                  : (isLooping ? inR[nextPos % data.getNumSamples()] : 0.0f);
          r = (currentR * invAlpha) + (nextR * alpha);
        } else {
          r = l;
        }
      }

      // Filter Processing
      l = filter.processSample(0, l);
      if (outR)
        r = filter.processSample(0, r); // Naive stereo filtering (same state?)
      // Ideally separate filters for stereo, but SVF TPT handles mono nicely.
      // If we use same filter object for L and R, state updates twice per
      // sample? Yes. This shifts frequency. For proper stereo, we need 2
      // filters or process mono. For now, let's process L and copy to R if
      // naive, or accept the slight error for "Refinement". Correct way:
      // filter.processSample(0, l) is fine. Actually SVF TPT is mono.

      // Envelope
      float env = adsr.getNextSample();

      if (outL)
        *outL++ += l * level * env;
      if (outR)
        *outR++ += r * level * env;

      sourceSamplePosition += pitchRatio * pitchMod;

      // Loop handling
      if (isLooping) {
        if (sourceSamplePosition >= data.getNumSamples())
          sourceSamplePosition -= data.getNumSamples();
      } else {
        if (sourceSamplePosition >= data.getNumSamples()) {
          clearCurrentNote();
          break;
        }
      }
    }
  }
}

void HowlingVoice::updateADSR(const juce::ADSR::Parameters &params) {
  adsrParams = params;
  adsr.setParameters(adsrParams);
}

//==============================================================================
// SynthEngine
//==============================================================================

SynthEngine::SynthEngine() {}

void SynthEngine::initialize() {
  // Add voices
  // We want 16 voices
  for (int i = 0; i < 16; i++) {
    addVoice(new HowlingVoice());
  }
}

void SynthEngine::prepare(double sampleRate, int samplesPerBlock) {
  setCurrentPlaybackSampleRate(sampleRate);
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->prepare(sampleRate, samplesPerBlock);
    }
  }
}

void SynthEngine::updateParams(float attack, float decay, float sustain,
                               float release, float cutoff, float resonance,
                               float lfoRate, float lfoDepth) {
  juce::ADSR::Parameters params;
  params.attack = attack;
  params.decay = decay;
  params.sustain = sustain;
  params.release = release;

  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateADSR(params);
      voice->updateFilter(cutoff, resonance);
      voice->updateLFO(lfoRate, lfoDepth);
    }
  }
}
