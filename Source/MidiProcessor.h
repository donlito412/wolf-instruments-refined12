#pragma once

#include <JuceHeader.h>

//==============================================================================
// Arpeggiator Module
//==============================================================================
class Arpeggiator {
public:
  Arpeggiator();
  ~Arpeggiator() = default;

  void prepare(double sampleRate);
  void process(juce::MidiBuffer &midiMessages, int numSamples,
               juce::AudioPlayHead *playHead);
  void reset();

  // Parameters
  void setParameters(float rate, int mode, int octaves, float gate, bool on);

private:
  double currentSampleRate = 44100.0;

  // State
  int currentNote = -1;
  int currentStep = 0;
  double noteTime = 0.0;
  std::vector<int> sortedNotes; // Held notes sorted

  struct ActiveNote {
    int noteNumber;
    int samplesRemaining;
  };
  std::vector<ActiveNote> activeNotes;

  // Params
  bool enabled = false;
  float rateDiv = 1.0f; // 1 = quarter note? No, standard divisions.
  int arpMode = 0;      // 0=Up, 1=Down, 2=Up/Down, 3=Random
  int numOctaves = 1;
  float gateLength = 0.5f;

  // Helper
  void handleNoteOn(int note, int velocity);
  void handleNoteOff(int note);
  int getNextNote();
  double getSamplesPerStep(juce::AudioPlayHead *playHead);
};

//==============================================================================
// Chord Engine Module
//==============================================================================
class ChordEngine {
public:
  ChordEngine();
  ~ChordEngine() = default;

  void process(juce::MidiBuffer &midiMessages);

  // Parameters
  void setParameters(int mode, int keys); // mode: 0=Off, 1=Maj, 2=Min...

private:
  int chordMode = 0;
  // Temporary storage for generated notes to ensure NoteOffs match
  // For v1, we transform NoteOn(C) -> NoteOn(C, E, G).
  // And NoteOff(C) -> NoteOff(C, E, G).
};

//==============================================================================
// MidiProcessor (Main Handler)
//==============================================================================
class MidiProcessor {
public:
  MidiProcessor();
  ~MidiProcessor() = default;

  void prepare(double sampleRate);
  void process(juce::MidiBuffer &midiMessages, int numSamples,
               juce::AudioPlayHead *playHead);
  void reset();

  // Accessors for Modules
  Arpeggiator &getArp() { return arp; }
  ChordEngine &getChordEngine() { return chordEngine; }

private:
  Arpeggiator arp;
  ChordEngine chordEngine;

  double currentSampleRate = 44100.0;
};
