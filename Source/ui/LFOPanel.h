#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "CurveEditor.h"
#include "CustomLFOTabButton.h"
#include "CustomLookAndFeel.h"
#include "ModulatableSlider.h"

// Forward declaration
class AudioPluginAudioProcessor;

class CustomLFOTabbedComponent : public juce::TabbedComponent
{
public:
    CustomLFOTabbedComponent() : juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop)
    {
    }
    
    juce::TabBarButton* createTabButton(const juce::String& tabName, int tabIndex) override
    {
        return new CustomLFOTabButton(tabName, getTabbedButtonBar(), tabIndex);
    }
};

class LFOPanel : public juce::Component,
                 public CurveEditor::Listener
{
public:
    LFOPanel(AudioPluginAudioProcessor& proc) : processor(proc)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            auto container = std::make_unique<juce::Component>();
            
            auto editor = std::make_unique<CurveEditor>();
            editor->setLFO(&processor.getLFO(i));
            container->addAndMakeVisible(editor.get());
            curveEditors.push_back(std::move(editor));
            
            auto modeBox = std::make_unique<juce::ComboBox>();
            modeBox->addItem("Trigger", 1);
            modeBox->addItem("Sync", 2);
            container->addAndMakeVisible(modeBox.get());
            modeComboBoxes.push_back(std::move(modeBox));
            
            auto modeLabel = std::make_unique<juce::Label>();
            modeLabel->setText("Mode", juce::dontSendNotification);
            modeLabel->setJustificationType(juce::Justification::centred);
            modeLabel->setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
            container->addAndMakeVisible(modeLabel.get());
            modeLabels.push_back(std::move(modeLabel));
            
            juce::String modeParamID = "lfo" + juce::String(i + 1) + "_mode";
            modeAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.apvts, modeParamID, *modeComboBoxes.back()));
            
            auto syncButton = std::make_unique<juce::ToggleButton>("SYNC");
            syncButton->getProperties().set("pillToggle", true);
            container->addAndMakeVisible(syncButton.get());
            syncButtons.push_back(std::move(syncButton));
            
            juce::String syncParamID = "lfo" + juce::String(i + 1) + "_sync";
            syncAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                processor.apvts, syncParamID, *syncButtons.back()));
            
            createModulatableControl(container.get(), i, "rate", "Rate", rateSliders, rateLabels, rateAttachments);
            createTensionControl(container.get(), i);
            
            // Add listener to sync button to switch parameter
            syncButtons.back()->onClick = [this, i]()
            {
                updateLFORateControl(i);
            };
            
            tabContainers.push_back(std::move(container));
            
            tabbedComponent.addTab("LFO " + juce::String(i + 1),
                                  juce::Colour(0xff1a1a1a),
                                  tabContainers.back().get(),
                                  false);
        }
        
        tabbedComponent.setTabBarDepth(40);
        tabbedComponent.setOutline(0);
        addAndMakeVisible(tabbedComponent);
        
        // Register as listeners, then load each LFO's saved curve + tension.
        for (size_t i = 0; i < 4; ++i)
            curveEditors[i]->addListener(this);

        reloadFromState();
    }
    
    ~LFOPanel() override
    {
        // Unregister from curve editors
        for (size_t i = 0; i < 4; ++i)
        {
            curveEditors[i]->removeListener(this);
        }
    }
    
    // Re-reads every LFO's saved curve + tension and re-syncs the audio LFOs. Also re-binds the
    // rate knob: the SYNC button's onClick can't do it on construction or a preset load, since a
    // ButtonAttachment only notifies when the toggle state actually changes.
    void reloadFromState()
    {
        for (size_t i = 0; i < 4; ++i)
        {
            auto savedPoints = processor.getLFOCurvePoints(i);
            curveEditors[i]->setControlPoints(savedPoints.empty()
                ? CurveEditor::defaultControlPoints()
                : savedPoints);

            juce::String tensionParamID = "lfo" + juce::String(i + 1) + "_tension";
            if (auto* param = processor.apvts.getParameter(tensionParamID))
                curveEditors[i]->setTension(param->getValue());

            syncCurveToLFO(curveEditors[i].get(), i);
            updateLFORateControl(i);
        }
    }

    // CurveEditor::Listener
    void curveChanged(CurveEditor* editor) override
    {
        // Find which LFO this curve editor belongs to
        for (size_t i = 0; i < 4; ++i)
        {
            if (curveEditors[i].get() == editor)
            {
                // Save the curve points to processor
                processor.setLFOCurvePoints(i, editor->getControlPoints());
                
                syncCurveToLFO(editor, i);
                break;
            }
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);
    }
    
    void resized() override
    {
        tabbedComponent.setBounds(getLocalBounds());
        
        // Layout each container: editor on top, controls below
        for (size_t i = 0; i < tabContainers.size(); ++i)
        {
            auto bounds = tabContainers[i]->getLocalBounds().reduced(10);
            
            // Curve editor takes top portion
            int editorHeight = juce::roundToInt(bounds.getHeight() * 0.7f);
            auto editorBounds = bounds.removeFromTop(editorHeight);
            curveEditors[i]->setBounds(editorBounds);
            
            bounds.removeFromTop(10); // Spacing
            
            auto controlsArea = bounds;
            
            auto modeArea = controlsArea.removeFromLeft(controlsArea.getWidth() / 2).reduced(5);
            modeLabels[i]->setBounds(modeArea.removeFromTop(16));
            modeComboBoxes[i]->setBounds(modeArea.removeFromTop(24));
            
            int knobSize = 50;
            int knobX = controlsArea.getX() + 10;
            int labelY = controlsArea.getY() - 10;
            int knobY = controlsArea.getY() + 5;

            rateLabels[i]->setBounds(knobX, labelY, knobSize, 16);
            rateSliders[i]->setBounds(knobX, knobY, knobSize, knobSize);
            
            int syncButtonWidth = 35;
            int syncButtonHeight = 12;
            syncButtons[i]->setBounds(knobX + knobSize + 10, knobY + 5, syncButtonWidth, syncButtonHeight);
            
            int tensionX = knobX + knobSize + 55;
            tensionLabels[i]->setBounds(tensionX, labelY, knobSize, 16);
            tensionSliders[i]->setBounds(tensionX, knobY, knobSize, knobSize);
        }
    }
    
private:
    void createTensionControl(juce::Component* container, size_t lfoIndex)
    {
        juce::String paramID = "lfo" + juce::String(lfoIndex + 1) + "_tension";
        
        auto slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        container->addAndMakeVisible(slider.get());
        tensionSliders.push_back(std::move(slider));
        tensionLabels.push_back(createLabel("Tension", container));
        
        tensionSliders.back()->onValueChange = [this, lfoIndex]()
        {
            curveEditors[lfoIndex]->setTension(static_cast<float>(tensionSliders[lfoIndex]->getValue()));
            syncCurveToLFO(curveEditors[lfoIndex].get(), lfoIndex);
        };
        
        tensionAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, paramID, *tensionSliders.back()));
    }
    
    void createModulatableControl(juce::Component* container, size_t lfoIndex,
                                   const juce::String& paramSuffix, const juce::String& labelText,
                                   std::vector<std::unique_ptr<ModulatableSlider>>& sliders,
                                   std::vector<std::unique_ptr<juce::Label>>& labels,
                                   std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>& attachments)
    {
        juce::String paramID = "lfo" + juce::String(lfoIndex + 1) + "_" + paramSuffix;
        
        auto slider = std::make_unique<ModulatableSlider>(processor, paramID);
        container->addAndMakeVisible(slider.get());
        sliders.push_back(std::move(slider));
        labels.push_back(createLabel(labelText, container));
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, paramID, *sliders.back()));
    }
    
    std::unique_ptr<juce::Label> createLabel(const juce::String& text, juce::Component* container)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText(text, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
        container->addAndMakeVisible(label.get());
        return label;
    }
    
    void syncCurveToLFO(CurveEditor* editor, size_t lfoIndex)
    {
        auto curveFunction = [editor](float x) { return editor->getValueAt(x); };
        processor.getLFO(lfoIndex).syncFromCurve(curveFunction);
    }
    
    void updateLFORateControl(size_t lfoIndex)
    {
        const bool syncEnabled = syncButtons[lfoIndex]->getToggleState();
        
        rateAttachments[lfoIndex].reset();

        juce::String paramID;
        if (syncEnabled)
        {
            paramID = "lfo" + juce::String(lfoIndex + 1) + "_sync_rate";
        }
        else
        {
            paramID = "lfo" + juce::String(lfoIndex + 1) + "_rate";
        }
        
        rateSliders[lfoIndex]->setParameterID(paramID);
        rateAttachments[lfoIndex] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, paramID, *rateSliders[lfoIndex]);
    }
    
    AudioPluginAudioProcessor& processor;
    CustomLFOTabbedComponent tabbedComponent;
    std::vector<std::unique_ptr<juce::Component>> tabContainers;
    std::vector<std::unique_ptr<CurveEditor>> curveEditors;
    std::vector<std::unique_ptr<juce::ComboBox>> modeComboBoxes;
    std::vector<std::unique_ptr<juce::Label>> modeLabels, rateLabels, tensionLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> modeAttachments;
    std::vector<std::unique_ptr<ModulatableSlider>> rateSliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> rateAttachments;
    std::vector<std::unique_ptr<juce::ToggleButton>> syncButtons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> syncAttachments;
    std::vector<std::unique_ptr<juce::Slider>> tensionSliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> tensionAttachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOPanel)
};
