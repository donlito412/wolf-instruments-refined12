#include "PresetManager.h"
#include "SampleManager.h"

const juce::File PresetManager::defaultDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Presets")};

const juce::File PresetManager::factoryDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Factory Presets")};

// Changed extension to .wav per user request
const juce::String PresetManager::presetExtension{".wav"};

const juce::File PresetManager::sharedDirectory{
    "/Users/Shared/Wolf Instruments"};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts,
                             SampleManager &sm)
    : valueTreeState(apvts), sampleManager(sm) {
  // Do NOT create directories automatically.
  // Rely on user having folders where they want them.
}

// We no longer enforce category subfolders.
// User can organize files as they wish.

void PresetManager::savePreset(const juce::String &presetName,
                               const juce::String &category) {
  // Saving WAV presets is not yet supported via this interface.
  // We could implement saving current settings + sound reference to XML if
  // needed in future.
  DBG("Saving presets not supported in WAV mode yet.");
}

void PresetManager::deletePreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  // Search in all folders
  const auto presetFile = getPresetFile(presetName);

  if (!presetFile.existsAsFile()) {
    DBG("Preset file " + presetName + " does not exist");
    return;
  }

  // Prevent deleting Factory Presets (Simple check)
  if (presetFile.isAChildOf(factoryDirectory)) {
    DBG("Cannot delete factory preset");
    return;
  }

  if (!presetFile.deleteFile()) {
    DBG("Preset file " + presetFile.getFullPathName() +
        " could not be deleted");
    return;
  }

  currentPresetName = "";
}

void PresetManager::loadPreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  const auto presetFile = getPresetFile(presetName);

  if (!presetFile.existsAsFile()) {
    DBG("Preset file " + presetName + " does not exist");
    return;
  }

  // Load WAV via SampleManager
  sampleManager.loadSound(presetFile);
  currentPresetName = presetName;
}

// Helper to find file across subfolders (User + Factory)
// Helper to find file across subfolders (User + Factory)
juce::File PresetManager::getPresetFile(const juce::String &presetName) const {
  // Helper to search a root directory recursively
  auto findInRoot = [&](const juce::File &root) -> juce::File {
    auto options = juce::File::TypesOfFileToFind::findFiles;
    // Recursive search
    auto allFiles = root.findChildFiles(options, true, "*");
    for (const auto &f : allFiles) {
      if (f.getFileNameWithoutExtension() == presetName) {
        // Check extension case-insensitive
        if (f.getFileExtension().equalsIgnoreCase(presetExtension))
          return f;
      }
    }
    return juce::File();
  };

  auto f = findInRoot(defaultDirectory);
  if (f.existsAsFile())
    return f;

  f = findInRoot(factoryDirectory);
  if (f.existsAsFile())
    return f;

  f = findInRoot(sharedDirectory);
  if (f.existsAsFile())
    return f;

  return juce::File();
}

int PresetManager::loadNextPreset() {
  const auto allPresets = getAllPresets();
  if (allPresets.isEmpty())
    return -1;

  int currentIndex = -1;
  // Find current index manually
  for (int i = 0; i < allPresets.size(); ++i) {
    if (allPresets[i].getFileNameWithoutExtension() == currentPresetName) {
      currentIndex = i;
      break;
    }
  }

  const auto nextIndex =
      currentIndex + 1 > allPresets.size() - 1 ? 0 : currentIndex + 1;
  loadPreset(allPresets[nextIndex].getFileNameWithoutExtension());
  return nextIndex;
}

int PresetManager::loadPreviousPreset() {
  const auto allPresets = getAllPresets();
  if (allPresets.isEmpty())
    return -1;

  int currentIndex = -1;
  for (int i = 0; i < allPresets.size(); ++i) {
    if (allPresets[i].getFileNameWithoutExtension() == currentPresetName) {
      currentIndex = i;
      break;
    }
  }

  const auto prevIndex =
      currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
  loadPreset(allPresets[prevIndex].getFileNameWithoutExtension());
  return prevIndex;
}

juce::File PresetManager::getPresetFolder() const { return defaultDirectory; }

juce::Array<juce::File> PresetManager::getAllPresets() const {
  juce::Array<juce::File> presets;

  auto options = juce::File::TypesOfFileToFind::findFiles;

  auto scanRoot = [&](const juce::File &root) {
    DBG("Scanning root: " + root.getFullPathName());
    if (!root.isDirectory()) {
      DBG("Root is not a directory: " + root.getFullPathName());
      return;
    }
    // Recursively find all files
    auto allFiles = root.findChildFiles(options, true, "*"); // Scan everything

    for (const auto &file : allFiles) {
      if (file.getFileExtension().equalsIgnoreCase(presetExtension)) {
        DBG("Found preset: " + file.getFullPathName());
        presets.add(file);
      }
    }
  };

  scanRoot(defaultDirectory);
  scanRoot(sharedDirectory); // Check Shared folder (Installer location)
  scanRoot(factoryDirectory);

  return presets;
}

juce::String PresetManager::getCurrentPreset() const {
  return currentPresetName;
}
