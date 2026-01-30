#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HowlingWolvesAudioProcessor::HowlingWolvesAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              // .withInput("Input", juce::AudioChannelSet::stereo(), true) //
              // Disabled to prevent feedback loop in Standalone
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      sampleManager(synthEngine), presetManager(apvts, sampleManager) {
  // Load initial samples
  sampleManager.loadSamples();
}

HowlingWolvesAudioProcessor::~HowlingWolvesAudioProcessor() {}

//==============================================================================
const juce::String HowlingWolvesAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool HowlingWolvesAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool HowlingWolvesAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool HowlingWolvesAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double HowlingWolvesAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HowlingWolvesAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int HowlingWolvesAudioProcessor::getCurrentProgram() { return 0; }

void HowlingWolvesAudioProcessor::setCurrentProgram(int /*index*/) {}

const juce::String HowlingWolvesAudioProcessor::getProgramName(int /*index*/) {
  return {};
}

void HowlingWolvesAudioProcessor::changeProgramName(
    int /*index*/, const juce::String & /*newName*/) {}

//==============================================================================
void HowlingWolvesAudioProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock) {
  synthEngine.setCurrentPlaybackSampleRate(sampleRate);
  synthEngine.prepare(sampleRate, samplesPerBlock);
  midiProcessor.prepare(sampleRate);

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = getTotalNumOutputChannels();

  effectsProcessor.prepare(spec);
}

void HowlingWolvesAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool HowlingWolvesAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void HowlingWolvesAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear the buffer to prevent static/garbage noise
  buffer.clear();

  // (Optional) If we wanted to keep input (like an effect plugin), we wouldn't
  // clear. But this is an Instrument. We must clear.

  // This loop is redundant if we use buffer.clear(), but keeping it for
  // safety/convention if logic changes
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // --- MIDI Processing Stage ---
  // 1. Process keyboard state (Input -> MidiBuffer)
  keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                      true);

  // 2. Perform Midi Transformation (Arp / Chords)
  midiProcessor.process(midiMessages, buffer.getNumSamples(), getPlayHead());

  // Read parameters from APVTS and apply to synth engine (Josh Hodge pattern)
  auto *attackParam = apvts.getRawParameterValue("attack");
  auto *decayParam = apvts.getRawParameterValue("decay");
  auto *sustainParam = apvts.getRawParameterValue("sustain");
  auto *releaseParam = apvts.getRawParameterValue("release");
  auto *filterCutoffParam = apvts.getRawParameterValue("filterCutoff");
  auto *filterResParam = apvts.getRawParameterValue("filterRes");
  auto *lfoRateParam = apvts.getRawParameterValue("lfoRate");
  auto *lfoDepthParam = apvts.getRawParameterValue("lfoDepth");

  // Output Parameters
  auto *gainParam = apvts.getRawParameterValue("gain");
  auto *panParam = apvts.getRawParameterValue("pan");

  // Effects Parameters
  auto *distDrive = apvts.getRawParameterValue("distDrive");
  auto *distMix = apvts.getRawParameterValue("distMix");
  auto *delayTime = apvts.getRawParameterValue("delayTime");
  auto *delayFdbk = apvts.getRawParameterValue("delayFeedback");
  auto *delayMix = apvts.getRawParameterValue("delayMix");
  auto *revSize = apvts.getRawParameterValue("reverbSize");
  auto *revDamp = apvts.getRawParameterValue("reverbDamping");
  auto *revMix = apvts.getRawParameterValue("REVERB_MIX");

  // Apply parameters to synth engine
  auto *filterTypeParam = apvts.getRawParameterValue("filterType");

  // Apply parameters to synth engine
  if (attackParam && decayParam && sustainParam && releaseParam &&
      filterCutoffParam && filterResParam && lfoRateParam && lfoDepthParam &&
      filterTypeParam) {

    int fType = (int)filterTypeParam->load();

    synthEngine.updateParams(
        attackParam->load(), decayParam->load(), sustainParam->load(),
        releaseParam->load(), filterCutoffParam->load(), filterResParam->load(),
        fType, lfoRateParam->load(), lfoDepthParam->load());
  }

  // --- Update Midi Processor ---
  auto *arpOn = apvts.getRawParameterValue("arpEnabled");
  auto *arpRate = apvts.getRawParameterValue("arpRate"); // 0=1/4, 1=1/8...
  auto *arpMode = apvts.getRawParameterValue("arpMode");
  auto *arpOct = apvts.getRawParameterValue("arpOctave");
  auto *arpGate = apvts.getRawParameterValue("arpGate");
  auto *chordMode = apvts.getRawParameterValue("chordMode");

  if (arpOn && arpRate && arpMode && arpOct && arpGate) {
    // Map Rate Index to float divisor logic (0 -> 0.0, 1 -> 0.3, 2 -> 0.6, 3 ->
    // 0.9) Arpeggiator logic uses float thresholds:
    // <=0.1 = 1/4, <=0.4 = 1/8, <=0.7 = 1/16, >0.7 = 1/32
    float rateVal = arpRate->load(); // This is index? No, getRawParameterValue
                                     // returns float index for Choices?
    // Yes, Choice returns index as float (0.0, 1.0, 2.0).
    // We need to map it.
    // 0.0 (1/4) -> 0.0
    // 1.0 (1/8) -> 0.3 (to trigger 1/8 logic)
    // 2.0 (1/16) -> 0.6
    // 3.0 (1/32) -> 0.9
    float driverVal = rateVal * 0.3f;

    bool isOn = *arpOn > 0.5f;
    midiProcessor.getArp().setParameters(driverVal, (int)arpMode->load(),
                                         (int)arpOct->load(), arpGate->load(),
                                         isOn);
  }

  if (chordMode) {
    midiProcessor.getChordEngine().setParameters((int)chordMode->load(), 0);
  }

  // --- Sample & Tune Parameters ---
  auto *tuneParam = apvts.getRawParameterValue("tune");
  auto *startParam = apvts.getRawParameterValue("sampleStart");
  auto *endParam = apvts.getRawParameterValue("sampleEnd");
  auto *loopParam = apvts.getRawParameterValue("sampleLoop");

  float tuneVal = tuneParam ? tuneParam->load() : 0.0f;
  float startVal = startParam ? startParam->load() : 0.0f;
  float endVal = endParam ? endParam->load() : 1.0f;
  bool loopVal = loopParam ? (loopParam->load() > 0.5f) : true;

  synthEngine.updateSampleParams(tuneVal, startVal, endVal, loopVal);

  // Apply parameters to effects processor
  // Use safe loading to prevent one missing parameter from breaking all effects
  // Diagonostic: Default mix to 0.0f (so if working, follows knob).
  // If pointer is NULL (broken), default to 0.0f?
  // User says "don't work".
  // I will check the Parameter IDs one last time manually.
  // Actually, I will revert to standard 0.0f defaults but ensure I didn't make
  // a typo.

  // Wait, I will use the defaults defined in CreateParameterLayout to be
  // consistent. Delay Time default 0.5. Feedback 0.3.

  float distDriveVal =
      distDrive ? distDrive->load() : 1.0f;            // Default MAX if broken
  float distMixVal = distMix ? distMix->load() : 0.5f; // Default AUDIBLE
  float delayTimeVal = delayTime ? delayTime->load() : 0.5f;
  float delayFdbkVal = delayFdbk ? delayFdbk->load() : 0.3f; // Default 0.3
  float delayMixVal = delayMix ? delayMix->load() : 0.5f;    // Default AUDIBLE
  float revSizeVal = revSize ? revSize->load() : 0.5f;
  float revDampVal = revDamp ? revDamp->load() : 0.5f;
  float revMixVal = revMix ? revMix->load() : 0.5f; // Default AUDIBLE

  float biteVal = 0.0f;
  if (auto *biteParam = apvts.getRawParameterValue("BITE"))
    biteVal = *biteParam;

  effectsProcessor.updateParameters(distDriveVal, distMixVal, delayTimeVal,
                                    delayFdbkVal, delayMixVal, revSizeVal,
                                    revDampVal, revMixVal, biteVal);

  // Update Chain Order
  auto *chainOrderParam = apvts.getRawParameterValue("CHAIN_ORDER");
  if (chainOrderParam) {
    int mode = (int)chainOrderParam->load();
    using ET = EffectsProcessor::EffectType;
    std::array<ET, 4> order;

    // Standard: Dist -> Bite -> Delay -> Reverb
    // Ethereal: Reverb -> Delay -> Dist -> Bite
    // Chaos: Delay -> Dist -> Bite -> Reverb
    // Reverse: Reverb -> Delay -> Bite -> Dist

    switch (mode) {
    default:
    case 0:
      order = {ET::Distortion, ET::TransientShaper, ET::Delay, ET::Reverb};
      break;
    case 1:
      order = {ET::Reverb, ET::Delay, ET::Distortion, ET::TransientShaper};
      break;
    case 2:
      order = {ET::Delay, ET::Distortion, ET::TransientShaper, ET::Reverb};
      break;
    case 3:
      order = {ET::Reverb, ET::Delay, ET::TransientShaper, ET::Distortion};
      break;
    }
    effectsProcessor.setChainOrder(order);
  }

  // Process synth
  synthEngine.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

  // Process effects
  effectsProcessor.process(buffer);

  // --- Master Section (Gain / Pan) ---
  float gain = gainParam ? gainParam->load() : 0.5f;
  float pan = panParam ? panParam->load() : 0.0f;

  // Apply Master Gain
  buffer.applyGain(gain);

  // Apply Master Pan (Constant Power)
  if (totalNumOutputChannels == 2) {
    // Pan range -1.0 to 1.0
    float angle = (pan + 1.0f) * (juce::MathConstants<float>::pi / 4.0f);
    float leftGain = std::cos(angle);
    float rightGain = std::sin(angle);

    buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
    buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
  }

  // Push to Visualizer
  if (audioVisualizerHook)
    audioVisualizerHook(buffer);
}

// Helper to update all params including new ones
// synthEngine.updateParams(...) needs update.
// I will do it in next step. For now volume works.

//==============================================================================
bool HowlingWolvesAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *HowlingWolvesAudioProcessor::createEditor() {
  return new HowlingWolvesAudioProcessorEditor(*this);
}

//==============================================================================
void HowlingWolvesAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {

  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void HowlingWolvesAudioProcessor::setStateInformation(const void *data,
                                                      int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(apvts.state.getType())) {
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
HowlingWolvesAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Example: Master Gain
  layout.add(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f,
                                                         1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("pan", "Pan", -1.0f,
                                                         1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("tune", "Tune", -12.0f,
                                                         12.0f, 0.0f));

  // Attack, Decay, Sustain, Release (Simple ADSR for now, can be expanded)
  layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
                                                         0.01f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("decay", "Decay",
                                                         0.01f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain",
                                                         0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("release", "Release",
                                                         0.01f, 5.0f, 0.1f));

  // Filter parameters
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "filterType", "Filter Type",
      juce::StringArray{"Low Pass", "High Pass", "Band Pass", "Notch"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterCutoff", "Filter Cutoff",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterRes", "Filter Resonance", 0.0f, 1.0f, 0.5f));

  // LFO parameters
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lfoWave", "LFO Waveform",
      juce::StringArray{"Sine", "Square", "Triangle"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>("lfoRate", "LFO Rate",
                                                         0.01f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lfoTarget", "LFO Target",
      juce::StringArray{"Filter Cutoff", "Volume", "Pan", "Pitch"}, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lfoDepth", "LFO Depth", 0.0f, 1.0f, 0.5f));

  // Sample Parameters (Added)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sampleStart", "Sample Start", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sampleEnd", "Sample End", 0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterBool>("sampleLoop",
                                                        "Sample Loop", true));

  // --- Effects Parameters ---

  // Distortion
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "distDrive", "Distortion Drive", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "distMix", "Distortion Mix", 0.0f, 1.0f, 0.0f));

  // Delay
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayTime", "Delay Time", 0.0f, 2.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f));

  // Reverb
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "reverbSize", "Reverb Size", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "reverbDamping", "Reverb Damping", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "REVERB_MIX", "Reverb Mix", 0.0f, 1.0f, 0.3f));

  // Transient Shaper
  layout.add(std::make_unique<juce::AudioParameterFloat>("BITE", "Bite Amount",
                                                         -1.0f, 1.0f, 0.0f));

  // --- MIDI Performance Parameters ---
  layout.add(std::make_unique<juce::AudioParameterBool>("arpEnabled", "Arp On",
                                                        false));
  // Rates: 0=1/4, 1=1/8, 2=1/16, 3=1/32
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "arpRate", "Arp Rate", juce::StringArray{"1/4", "1/8", "1/16", "1/32"},
      1)); // Default 1/8

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "arpMode", "Arp Mode",
      juce::StringArray{"Up", "Down", "Up/Down", "Random"}, 0));

  layout.add(std::make_unique<juce::AudioParameterInt>("arpOctave",
                                                       "Arp Octaves", 1, 4, 1));
  layout.add(std::make_unique<juce::AudioParameterFloat>("arpGate", "Arp Gate",
                                                         0.1f, 1.0f, 0.5f));

  // Chord Mode
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "chordMode", "Chord Mode",
      juce::StringArray{"Off", "Major", "Minor", "7th", "9th"}, 0));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "HUNT_MODE", "Hunt Mode", juce::StringArray{"Stalk", "Chase", "Kill"},
      0));

  // Signal Chain Order
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "CHAIN_ORDER", "Signal Chain",
      juce::StringArray{"Standard", "Ethereal", "Chaos", "Reverse"}, 0));

  return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new HowlingWolvesAudioProcessor();
}
