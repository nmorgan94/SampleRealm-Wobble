#pragma once

#include "PluginProcessor.h"

// Handles user preset files on disk. State (de)serialization itself is delegated to the
// processor's shared getStateTree()/applyStateTree() helpers, so a preset file is exactly
// the same XML the DAW embeds for project state — just stored as a .wobble file.
class PresetManager
{
public:
    static inline const juce::String fileExtension { ".wobble" };
    static inline const juce::String fileWildcard  { "*.wobble" };

    explicit PresetManager(AudioPluginAudioProcessor& proc) : processor(proc)
    {
        auto dir = presetDirectory();
        if (! dir.exists())
            dir.createDirectory();
    }

    // ~/Library/Application Support/SampleRealm/Wobble/Presets (per the user app data dir).
    static juce::File presetDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("SampleRealm")
                   .getChildFile("Wobble")
                   .getChildFile("Presets");
    }

    juce::StringArray getPresetList() const
    {
        juce::StringArray names;
        for (auto& file : presetDirectory().findChildFiles(juce::File::findFiles, false, fileWildcard))
            names.add(file.getFileNameWithoutExtension());

        names.sort(true);
        return names;
    }

    bool savePreset(const juce::String& name)
    {
        if (name.isEmpty())
            return false;

        if (auto xml = processor.getStateTree().createXml())
        {
            if (xml->writeTo(presetFile(name)))
            {
                currentPreset = name;
                return true;
            }
        }

        return false;
    }

    bool loadPreset(const juce::String& name)
    {
        auto file = presetFile(name);
        if (! file.existsAsFile())
            return false;

        if (auto xml = juce::XmlDocument::parse(file))
        {
            if (processor.applyStateTree(juce::ValueTree::fromXml(*xml)))
            {
                currentPreset = name;
                return true;
            }
        }

        return false;
    }

    // Moves the preset file to the system Trash (recoverable). Does not alter the current
    // sound - it just removes the saved copy. Clears currentPreset if it was the one deleted.
    bool deletePreset(const juce::String& name)
    {
        auto file = presetFile(name);
        if (! file.existsAsFile())
            return false;

        const bool deleted = file.moveToTrash();
        if (deleted && name == currentPreset)
            currentPreset = {};

        return deleted;
    }

    void loadInit()
    {
        processor.resetToDefaultState();
        currentPreset = {};
    }

    const juce::String& getCurrentPreset() const { return currentPreset; }

private:
    juce::File presetFile(const juce::String& name) const
    {
        return presetDirectory().getChildFile(name + fileExtension);
    }

    AudioPluginAudioProcessor& processor;
    juce::String currentPreset;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
