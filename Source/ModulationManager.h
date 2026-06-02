#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>
#include <unordered_map>

class ModulationManager : private juce::ValueTree::Listener
{
public:
    enum class ModulationSource
    {
        None = 0,
        LFO,
        Envelope
    };
    
    struct ModulationAssignment
    {
        ModulationSource sourceType = ModulationSource::None;
        int sourceIndex = -1;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        
        bool isAssigned() const { return sourceType != ModulationSource::None && sourceIndex >= 0 && sourceIndex < 4; }
        bool isLFO() const { return sourceType == ModulationSource::LFO; }
        bool isEnvelope() const { return sourceType == ModulationSource::Envelope; }
    };
    
    ModulationManager() { rebuildSnapshot(); }

    ~ModulationManager() override
    {
        if (state.isValid())
            state.removeListener(this);
    }

    void initialise(juce::ValueTree treeState, juce::UndoManager* treeUndoManager)
    {
        if (state.isValid())
            state.removeListener(this);

        state = treeState;
        undoManager = treeUndoManager;

        if (state.isValid())
            state.addListener(this);

        rebuildSnapshot();
    }
    
    // Apply an assignment struct directly (clears the routing when unassigned).
    void setAssignment(const juce::String& parameterID, const ModulationAssignment& assignment)
    {
        if (assignment.isAssigned())
            setAssignment(parameterID, assignment.sourceType, assignment.sourceIndex,
                          assignment.minValue, assignment.maxValue);
        else
            clearAssignment(parameterID);
    }

    void clearAssignment(const juce::String& parameterID)
    {
        auto assignmentNode = findAssignment(parameterID);
        if (assignmentNode.isValid())
            state.removeChild(assignmentNode, undoManager);
    }
    
    // Copies the assignment out from under a short lock, so this is safe to call
    // from the audio thread (it never holds a reference to the snapshot).
    ModulationAssignment getAssignment(const juce::String& parameterID) const
    {
        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        if (snapshot)
        {
            auto it = snapshot->assignments.find(parameterID);
            if (it != snapshot->assignments.end())
                return it->second;
        }
        return {};
    }
    
    float calculateModulatedValue(float sourceValue, float minValue, float maxValue) const
    {
        return juce::jlimit(0.0f, 1.0f, minValue + sourceValue * (maxValue - minValue));
    }

    static ModulationAssignment makeDefaultRange(ModulationSource sourceType, int sourceIndex,
                                                 float base, float depth = 0.25f)
    {
        ModulationAssignment assignment;
        assignment.sourceType = sourceType;
        assignment.sourceIndex = sourceIndex;

        if (sourceType == ModulationSource::Envelope)
        {
            assignment.minValue = base;
            assignment.maxValue = juce::jmin(1.0f, base + depth);
        }
        else
        {
            assignment.minValue = juce::jlimit(0.0f, 1.0f, base - depth);
            assignment.maxValue = juce::jmin(1.0f, base + depth);
        }

        return assignment;
    }

    static void applyDepthChange(ModulationAssignment& assignment, float base, float delta)
    {
        if (assignment.isEnvelope())
        {
            const float roomAbove = juce::jmax(0.0f, 1.0f - base);
            const float desiredDepth = (assignment.maxValue - assignment.minValue) + delta;
            const float depth = juce::jlimit(juce::jmin(0.05f, roomAbove), roomAbove, desiredDepth);

            assignment.minValue = base;
            assignment.maxValue = base + depth;
        }
        else
        {
            const float currentDepth = (assignment.maxValue - assignment.minValue) * 0.5f;
            const float desiredDepth = juce::jlimit(0.05f, 1.0f, currentDepth + delta);
            const float roomBelow = base, roomAbove = 1.0f - base;

            if (desiredDepth <= juce::jmin(roomBelow, roomAbove))
            {
                assignment.minValue = base - desiredDepth;
                assignment.maxValue = base + desiredDepth;
            }
            else
            {
                const float actualDepth = juce::jmin(desiredDepth, juce::jmax(roomBelow, roomAbove));
                assignment.minValue = juce::jmax(0.0f, base - actualDepth);
                assignment.maxValue = juce::jmin(1.0f, base + actualDepth);
            }
        }
    }

    bool isLFOAssigned(int lfoIndex) const
    {
        if (! isValidSourceIndex(lfoIndex))
            return false;

        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        return snapshot && snapshot->lfoAssigned[(size_t) lfoIndex];
    }

    bool isEnvelopeAssigned(int envIndex) const
    {
        if (! isValidSourceIndex(envIndex))
            return false;

        const juce::SpinLock::ScopedLockType sl(snapshotLock);
        return snapshot && snapshot->envAssigned[(size_t) envIndex];
    }

private:
    struct IDs
    {
        static inline const juce::Identifier assignment { "Assignment" };
        static inline const juce::Identifier parameterID { "parameterID" };
        static inline const juce::Identifier sourceType { "sourceType" };
        static inline const juce::Identifier sourceIndex { "sourceIndex" };
        static inline const juce::Identifier minValue { "minValue" };
        static inline const juce::Identifier maxValue { "maxValue" };
    };
    
    static bool isValidSourceIndex(int index)
    {
        return index >= 0 && index < 4;
    }
    
    ModulationAssignment toAssignment(juce::ValueTree assignmentNode) const
    {
        if (! assignmentNode.isValid())
            return {};
        
        return {
            static_cast<ModulationSource>(static_cast<int>(assignmentNode.getProperty(IDs::sourceType, 0))),
            static_cast<int>(assignmentNode.getProperty(IDs::sourceIndex, -1)),
            static_cast<float>(assignmentNode.getProperty(IDs::minValue, 0.0f)),
            static_cast<float>(assignmentNode.getProperty(IDs::maxValue, 1.0f))
        };
    }
    
    juce::ValueTree findAssignment(const juce::String& parameterID) const
    {
        if (! state.isValid())
            return {};
        
        for (auto child : state)
        {
            if (child.hasType(IDs::assignment)
                && child[IDs::parameterID].toString() == parameterID)
                return child;
        }
        
        return {};
    }
    
    void setAssignment(const juce::String& parameterID, ModulationSource sourceType, int sourceIndex, float minValue, float maxValue)
    {
        if (! state.isValid())
            return;
        
        auto assignmentNode = findAssignment(parameterID);
        if (! assignmentNode.isValid())
        {
            assignmentNode = juce::ValueTree(IDs::assignment);
            assignmentNode.setProperty(IDs::parameterID, parameterID, undoManager);
            state.addChild(assignmentNode, -1, undoManager);
        }
        
        assignmentNode.setProperty(IDs::sourceType, static_cast<int>(sourceType), undoManager);
        assignmentNode.setProperty(IDs::sourceIndex, sourceIndex, undoManager);
        assignmentNode.setProperty(IDs::minValue, minValue, undoManager);
        assignmentNode.setProperty(IDs::maxValue, maxValue, undoManager);
    }

    struct StringHash
    {
        size_t operator() (const juce::String& s) const noexcept { return (size_t) s.hashCode64(); }
    };

    using AssignmentMap = std::unordered_map<juce::String, ModulationAssignment, StringHash>;

    struct Snapshot
    {
        AssignmentMap assignments;
        std::array<bool, 4> lfoAssigned {};
        std::array<bool, 4> envAssigned {};
    };

    // Rebuilt on the message thread (direct edits via the ValueTree::Listener,
    // and preset loads via initialise) and published under the SpinLock.
    void rebuildSnapshot()
    {
        auto snap = std::make_shared<Snapshot>();

        if (state.isValid())
        {
            for (auto child : state)
            {
                if (! child.hasType(IDs::assignment))
                    continue;

                auto assignment = toAssignment(child);
                snap->assignments[child[IDs::parameterID].toString()] = assignment;

                if (assignment.isAssigned())
                {
                    if (assignment.isLFO())
                        snap->lfoAssigned[(size_t) assignment.sourceIndex] = true;
                    else if (assignment.isEnvelope())
                        snap->envAssigned[(size_t) assignment.sourceIndex] = true;
                }
            }
        }

        std::shared_ptr<const Snapshot> old;
        {
            const juce::SpinLock::ScopedLockType sl(snapshotLock);
            old = std::move(snapshot);
            snapshot = std::move(snap);
        }
        // 'old' is released here, on the message thread, outside the lock. The
        // audio thread never holds a snapshot reference, so it can never be the
        // one to free a retired snapshot.
    }

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override { rebuildSnapshot(); }
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override { rebuildSnapshot(); }
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override { rebuildSnapshot(); }
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override { rebuildSnapshot(); }
    void valueTreeParentChanged(juce::ValueTree&) override { rebuildSnapshot(); }

    juce::ValueTree state;
    juce::UndoManager* undoManager = nullptr;
    std::shared_ptr<const Snapshot> snapshot;
    mutable juce::SpinLock snapshotLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};
