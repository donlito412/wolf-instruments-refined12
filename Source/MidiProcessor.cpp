#include "MidiProcessor.h"

//==============================================================================
// Arpeggiator
//==============================================================================

Arpeggiator::Arpeggiator() {}

void Arpeggiator::prepare(double sampleRate) { currentSampleRate = sampleRate; }

void Arpeggiator::reset() {
  sortedNotes.clear();
  currentStep = 0;
  noteTime = 0.0;
  // Panic: Clear active notes to stop stuck ones on reset
  activeNotes.clear();
}

void Arpeggiator::setParameters(float rate, int mode, int octaves, float gate,
                                bool on) {
  rateDiv = rate;
  arpMode = mode;
  numOctaves = octaves;
  gateLength = gate;
  enabled = on;
}

void Arpeggiator::handleNoteOn(int note, int velocity) {
  bool wasEmpty = sortedNotes.empty();

  // Add unique and sort
  bool found = false;
  for (int n : sortedNotes)
    if (n == note)
      found = true;

  if (!found) {
    sortedNotes.push_back(note);
    std::sort(sortedNotes.begin(), sortedNotes.end());
  }

  // Instant Trigger: If this is the first note, force trigger ASAP
  if (wasEmpty) {
    currentStep = 0;
    noteTime = 1000000.0;
  }
}

void Arpeggiator::handleNoteOff(int note) {
  // Remove
  auto it = std::remove(sortedNotes.begin(), sortedNotes.end(), note);
  sortedNotes.erase(it, sortedNotes.end());
}

int Arpeggiator::getNextNote() {
  if (sortedNotes.empty())
    return -1;

  // Multi-Octave Logic
  int numNotes = sortedNotes.size();
  int totalSteps = numNotes * numOctaves;

  if (totalSteps == 0)
    return -1; // Safety

  int wrappedStep = currentStep % totalSteps;

  int noteIndex = wrappedStep % numNotes;
  int octaveOffset = wrappedStep / numNotes;

  int note = sortedNotes[noteIndex] + (octaveOffset * 12);

  // Range check (0-127)
  if (note > 127)
    note = 127;

  return note;
}

double Arpeggiator::getSamplesPerStep(juce::AudioPlayHead *playHead) {
  double bpm = 120.0;
  if (playHead) {
    if (auto pos = playHead->getPosition()) {
      if (pos->getBpm().hasValue())
        bpm = *pos->getBpm();
    }
  }

  // Safety Clamp
  if (bpm < 20.0)
    bpm = 120.0;

  // RateDiv: 0=1/4, 1=1/8...
  double quarterNoteSamples = (60.0 / bpm) * currentSampleRate;

  if (rateDiv <= 0.1f)
    return quarterNoteSamples; // 1/4
  if (rateDiv <= 0.4f)
    return quarterNoteSamples / 2.0; // 1/8
  if (rateDiv <= 0.7f)
    return quarterNoteSamples / 4.0; // 1/16
  return quarterNoteSamples / 8.0;   // 1/32
}

void Arpeggiator::process(juce::MidiBuffer &midiMessages, int numSamples,
                          juce::AudioPlayHead *playHead) {

  // --- 1. Handle Active Notes (Pending Offs) if DISABLED ---
  if (!enabled) {
    for (auto it = activeNotes.begin(); it != activeNotes.end();) {
      int noteOffPos = it->samplesRemaining;
      if (noteOffPos < numSamples) {
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, it->noteNumber),
                              noteOffPos);
        it = activeNotes.erase(it);
      } else {
        it->samplesRemaining -= numSamples;
        ++it;
      }
    }
    return;
  }

  // --- 2. Process Input (Capture Input Notes, Filter them out) ---
  juce::MidiBuffer processedMidi; // This will become our Output Buffer

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      handleNoteOn(msg.getNoteNumber(), msg.getVelocity());
    } else if (msg.isNoteOff()) {
      handleNoteOff(msg.getNoteNumber());
    } else if (msg.isAllNotesOff()) {
      reset();
    } else {
      // Pass thru control changes, pitch bend, etc.
      processedMidi.addEvent(msg, metadata.samplePosition);
    }
  }

  // Now 'processedMidi' contains everything EXCEPT the input notes.
  // We will append our generated notes (and pending offs) to THIS buffer.
  // Then we swap it into 'midiMessages' at the very end.

  // --- 3. Handle Active Notes (Pending Offs) ---
  // Append to processedMidi so they go out!
  for (auto it = activeNotes.begin(); it != activeNotes.end();) {
    int noteOffPos = it->samplesRemaining;

    if (noteOffPos < numSamples) {
      processedMidi.addEvent(juce::MidiMessage::noteOff(1, it->noteNumber),
                             noteOffPos);
      it = activeNotes.erase(it);
    } else {
      it->samplesRemaining -= numSamples;
      ++it;
    }
  }

  // --- 4. Generate Arp Notes ---
  if (sortedNotes.empty()) {
    midiMessages.swapWith(processedMidi); // Output cleaned buffer
    return;
  }

  double samplesPerStep = getSamplesPerStep(playHead);
  if (samplesPerStep < 100.0)
    samplesPerStep = 10000.0;

  if (noteTime > samplesPerStep)
    noteTime = samplesPerStep;

  int samplesRemaining = numSamples;
  int currentSamplePos = 0;

  while (samplesRemaining > 0) {
    if (noteTime >= samplesPerStep) {
      noteTime -= samplesPerStep;

      int noteToPlay = getNextNote();

      if (noteToPlay > 0) {
        // Add Note On to output
        processedMidi.addEvent(
            juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8)100),
            currentSamplePos);

        int gateSamples = static_cast<int>(samplesPerStep * gateLength);

        if (currentSamplePos + gateSamples < numSamples) {
          // Note Off fits in this block
          processedMidi.addEvent(juce::MidiMessage::noteOff(1, noteToPlay),
                                 currentSamplePos + gateSamples);
        } else {
          // Note Off is in future block
          ActiveNote an;
          an.noteNumber = noteToPlay;
          an.samplesRemaining = gateSamples - (numSamples - currentSamplePos);
          activeNotes.push_back(an);
        }
      }
      currentStep++;
    }

    int processAmount = std::min(samplesRemaining, 32);
    noteTime += processAmount;
    samplesRemaining -= processAmount;
    currentSamplePos += processAmount;
  }

  // Final Swap: Use our constructed buffer as the output
  midiMessages.swapWith(processedMidi);
}

//==============================================================================
// ChordEngine
//==============================================================================

ChordEngine::ChordEngine() {}

void ChordEngine::setParameters(int mode, int keys) { chordMode = mode; }

void ChordEngine::process(juce::MidiBuffer &midiMessages) {
  if (chordMode == 0)
    return; // Off

  juce::MidiBuffer processedBuf;

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();

    if (msg.isNoteOn() || msg.isNoteOff()) {
      int root = msg.getNoteNumber();
      int vel = msg.getVelocity();
      bool isOn = msg.isNoteOn();

      auto addEvent = [&](int note) {
        if (isOn)
          processedBuf.addEvent(
              juce::MidiMessage::noteOn(1, note, (juce::uint8)vel),
              metadata.samplePosition);
        else
          processedBuf.addEvent(juce::MidiMessage::noteOff(1, note),
                                metadata.samplePosition);
      };

      addEvent(root);

      switch (chordMode) {
      case 1: // Major
        addEvent(root + 4);
        addEvent(root + 7);
        break;
      case 2: // Minor
        addEvent(root + 3);
        addEvent(root + 7);
        break;
      case 3: // 7th
        addEvent(root + 4);
        addEvent(root + 7);
        addEvent(root + 10);
        break;
      case 4: // 9th
        addEvent(root + 4);
        addEvent(root + 7);
        addEvent(root + 14);
        break;
      default:
        break;
      }

    } else {
      processedBuf.addEvent(msg, metadata.samplePosition);
    }
  }

  midiMessages.swapWith(processedBuf);
}

//==============================================================================
// MidiProcessor
//==============================================================================

MidiProcessor::MidiProcessor() {}

void MidiProcessor::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  arp.prepare(sampleRate);
}

void MidiProcessor::reset() { arp.reset(); }

void MidiProcessor::process(juce::MidiBuffer &midiMessages, int numSamples,
                            juce::AudioPlayHead *playHead) {
  // 1. Chords First
  chordEngine.process(midiMessages);

  // 2. Arp Second
  arp.process(midiMessages, numSamples, playHead);
}
