#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PresetManager.h"

// Header-bar widget: a combo box listing user presets plus Init / Save / Save As actions, and
// a Delete item that appears only when a preset is loaded. Owns the PresetManager (the
// UI-agnostic file + state service). After a load or Init it fires onPresetLoaded so the editor
// can refresh the panels that the parameter attachments don't cover (modulation badges, drawn
// LFO curves).
class PresetBar : public juce::Component
{
public:
    explicit PresetBar(AudioPluginAudioProcessor& proc)
        : presetManager(proc)
    {
        selector.setTextWhenNothingSelected("Init");
        selector.onChange = [this]() { handleComboChange(); };
        addAndMakeVisible(selector);
        refreshList();
    }

    // Set by the editor: invoked after a preset is loaded or Init is applied.
    std::function<void()> onPresetLoaded;

    void resized() override
    {
        selector.setBounds(getLocalBounds());
    }

private:
    // Reserved combo item IDs for the fixed action entries; preset files start at PresetIdBase.
    enum MenuId
    {
        ItemInit = 1,
        ItemSave,
        ItemSaveAs,
        ItemDelete,
        PresetIdBase = 100
    };

    void refreshList()
    {
        selector.clear(juce::dontSendNotification);

        selector.addItem("Init",        ItemInit);
        selector.addItem("Save",        ItemSave);
        selector.addItem("Save As...",  ItemSaveAs);

        const auto current = presetManager.getCurrentPreset();
        if (current.isNotEmpty())
            selector.addItem("Delete \"" + current + "\"", ItemDelete);

        selector.addSeparator();

        int id = PresetIdBase;
        for (const auto& name : presetManager.getPresetList())
            selector.addItem(name, id++);

        selectCurrentInCombo();
    }

    void handleComboChange()
    {
        const int id = selector.getSelectedId();

        switch (id)
        {
            case ItemInit:
                presetManager.loadInit();
                notifyLoaded();
                return;

            case ItemSave:
                saveCurrentOrPrompt();
                break;

            case ItemSaveAs:
                showSaveAsDialog();
                break;

            case ItemDelete:
                showDeleteConfirmation();
                break;

            default:
                if (id >= PresetIdBase)
                {
                    if (presetManager.loadPreset(selector.getText()))
                        notifyLoaded();
                    else
                        selectCurrentInCombo();  // load failed - revert to the active preset
                    return;
                }
                break;
        }

        // Action items shouldn't linger as the box's selection - restore the active preset label.
        selectCurrentInCombo();
    }

    void saveCurrentOrPrompt()
    {
        const auto current = presetManager.getCurrentPreset();
        if (current.isNotEmpty())
        {
            presetManager.savePreset(current);
            refreshList();
        }
        else
        {
            showSaveAsDialog();
        }
    }

    void showSaveAsDialog()
    {
        saveDialog = std::make_unique<juce::AlertWindow>(
            "Save Preset", "Enter a name for this preset:", juce::MessageBoxIconType::NoIcon);

        saveDialog->addTextEditor("name", presetManager.getCurrentPreset());
        saveDialog->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
        saveDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        saveDialog->setVisible(true);
        juce::Component::SafePointer<PresetBar> safeThis(this);
        saveDialog->enterModalState(true, juce::ModalCallbackFunction::create(
            [safeThis](int result)
            {
                // The editor (and this PresetBar) may have been destroyed while the
                // modal was open - the deferred callback must not touch a dead object.
                if (safeThis == nullptr)
                    return;

                const auto name = safeThis->saveDialog->getTextEditorContents("name").trim();
                safeThis->saveDialog->setVisible(false);

                if (result == 1 && name.isNotEmpty())
                {
                    safeThis->presetManager.savePreset(name);
                    safeThis->refreshList();
                }
            }), false);
    }

    void showDeleteConfirmation()
    {
        const auto name = presetManager.getCurrentPreset();
        if (name.isEmpty())
            return;

        juce::Component::SafePointer<PresetBar> safeThis(this);
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            "Delete Preset",
            "Delete preset \"" + name + "\"?\nIt will be moved to the Trash.",
            "Delete", "Cancel", this,
            juce::ModalCallbackFunction::create(
                [safeThis, name](int result)
                {
                    if (safeThis != nullptr && result == 1 && safeThis->presetManager.deletePreset(name))
                        safeThis->refreshList();
                }));
    }

    void selectCurrentInCombo()
    {
        const auto current = presetManager.getCurrentPreset();

        if (current.isNotEmpty())
        {
            for (int i = 0; i < selector.getNumItems(); ++i)
            {
                // Only match preset rows (id >= PresetIdBase), never an action row that
                // happens to share the name (e.g. a preset literally named "Save").
                if (selector.getItemId(i) >= PresetIdBase && selector.getItemText(i) == current)
                {
                    selector.setSelectedItemIndex(i, juce::dontSendNotification);
                    return;
                }
            }
        }

        // No active preset (e.g. after Init): clear selection so the "Init" placeholder shows.
        selector.setSelectedId(0, juce::dontSendNotification);
    }

    void notifyLoaded()
    {
        // Rebuild the menu (not just re-select) so the conditional Delete item tracks the
        // new current preset - load makes it appear, Init makes it disappear.
        refreshList();
        if (onPresetLoaded)
            onPresetLoaded();
    }

    PresetManager presetManager;
    juce::ComboBox selector;
    std::unique_ptr<juce::AlertWindow> saveDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};
