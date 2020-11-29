/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Common.h"
#include "ProjectTimelineDiffLogic.h"
#include "ProjectTimeline.h"
#include "AnnotationsSequence.h"
#include "TimeSignaturesSequence.h"
#include "KeySignaturesSequence.h"

// TODO refactor, lots of duplicated code
namespace VCS
{

static SerializedData mergeAnnotationsAdded(const SerializedData &state, const SerializedData &changes);
static SerializedData mergeAnnotationsRemoved(const SerializedData &state, const SerializedData &changes);
static SerializedData mergeAnnotationsChanged(const SerializedData &state, const SerializedData &changes);

static SerializedData mergeTimeSignaturesAdded(const SerializedData &state, const SerializedData &changes);
static SerializedData mergeTimeSignaturesRemoved(const SerializedData &state, const SerializedData &changes);
static SerializedData mergeTimeSignaturesChanged(const SerializedData &state, const SerializedData &changes);

static SerializedData mergeKeySignaturesAdded(const SerializedData &state, const SerializedData &changes);
static SerializedData mergeKeySignaturesRemoved(const SerializedData &state, const SerializedData &changes);
static SerializedData mergeKeySignaturesChanged(const SerializedData &state, const SerializedData &changes);

static Array<DeltaDiff> createAnnotationsDiffs(const SerializedData &state, const SerializedData &changes);
static Array<DeltaDiff> createTimeSignaturesDiffs(const SerializedData &state, const SerializedData &changes);
static Array<DeltaDiff> createKeySignaturesDiffs(const SerializedData &state, const SerializedData &changes);

static void deserializeTimelineChanges(const SerializedData &state, const SerializedData &changes,
    OwnedArray<MidiEvent> &stateEvents, OwnedArray<MidiEvent> &changesEvents);

static DeltaDiff serializeTimelineChanges(Array<const MidiEvent *> changes,
    const String &description, int64 numChanges, const Identifier &deltaType);

static SerializedData serializeTimelineSequence(Array<const MidiEvent *> changes, const Identifier &tag);

static bool checkIfDeltaIsAnnotationType(const Delta *delta);
static bool checkIfDeltaIsTimeSignatureType(const Delta *delta);
static bool checkIfDeltaIsKeySignatureType(const Delta *delta);

ProjectTimelineDiffLogic::ProjectTimelineDiffLogic(TrackedItem &targetItem) :
    DiffLogic(targetItem) {}

const Identifier ProjectTimelineDiffLogic::getType() const
{
    return Serialization::Core::projectTimeline;
}

Diff *ProjectTimelineDiffLogic::createDiff(const TrackedItem &initialState) const
{
    using namespace Serialization::VCS;
    
    auto *diff = new Diff(this->target);

    for (int i = 0; i < this->target.getNumDeltas(); ++i)
    {
        const Delta *myDelta = this->target.getDelta(i);

        const auto myDeltaData(this->target.getDeltaData(i));
        SerializedData stateDeltaData;

        bool deltaFoundInState = false;
        bool dataHasChanged = false;

        for (int j = 0; j < initialState.getNumDeltas(); ++j)
        {
            const Delta *stateDelta = initialState.getDelta(j);

            if (myDelta->hasType(stateDelta->getType()))
            {
                stateDeltaData = initialState.getDeltaData(j);
                deltaFoundInState = (stateDeltaData.isValid());
                dataHasChanged = (! myDeltaData.isEquivalentTo(stateDeltaData));
                break;
            }
        }

        if (!deltaFoundInState || dataHasChanged)
        {
            if (myDelta->hasType(ProjectTimelineDeltas::annotationsAdded))
            {
                diff->applyDeltas(createAnnotationsDiffs(stateDeltaData, myDeltaData));
            }
            else if (myDelta->hasType(ProjectTimelineDeltas::timeSignaturesAdded))
            {
                diff->applyDeltas(createTimeSignaturesDiffs(stateDeltaData, myDeltaData));
            }
            else if (myDelta->hasType(ProjectTimelineDeltas::keySignaturesAdded))
            {
                diff->applyDeltas(createKeySignaturesDiffs(stateDeltaData, myDeltaData));
            }
        }
    }

    return diff;
}

Diff *ProjectTimelineDiffLogic::createMergedItem(const TrackedItem &initialState) const
{
    using namespace Serialization::VCS;
    auto *diff = new Diff(this->target);

    // step 1:
    // the default policy is merging all changes
    // from changes into target (of corresponding types)
    for (int i = 0; i < initialState.getNumDeltas(); ++i)
    {
        const Delta *stateDelta = initialState.getDelta(i);
        const auto stateDeltaData(initialState.getDeltaData(i));

        // for every supported type we need to spit out 
        // a delta of type eventsAdded with all events merged in there
        UniquePointer<Delta> annotationsDelta(
            new Delta(DeltaDescription(Serialization::VCS::headStateDelta),
                      ProjectTimelineDeltas::annotationsAdded));

        UniquePointer<Delta> timeSignaturesDelta(
            new Delta(DeltaDescription(Serialization::VCS::headStateDelta),
                      ProjectTimelineDeltas::timeSignaturesAdded));

        UniquePointer<Delta> keySignaturesDelta(
            new Delta(DeltaDescription(Serialization::VCS::headStateDelta),
                ProjectTimelineDeltas::keySignaturesAdded));

        SerializedData annotationsDeltaData;
        SerializedData timeSignaturesDeltaData;
        SerializedData keySignaturesDeltaData;

        bool deltaFoundInChanges = false;

        for (int j = 0; j < this->target.getNumDeltas(); ++j)
        {
            const Delta *targetDelta = this->target.getDelta(j);
            const auto targetDeltaData(this->target.getDeltaData(j));

            const bool bothDeltasAreAnnotationType =
                checkIfDeltaIsAnnotationType(stateDelta) && checkIfDeltaIsAnnotationType(targetDelta);

            const bool bothDeltasAreTimeSignatureType =
                checkIfDeltaIsTimeSignatureType(stateDelta) && checkIfDeltaIsTimeSignatureType(targetDelta);

            const bool bothDeltasAreKeySignatureType =
                checkIfDeltaIsKeySignatureType(stateDelta) && checkIfDeltaIsKeySignatureType(targetDelta);

            if (bothDeltasAreAnnotationType)
            {
                deltaFoundInChanges = true;
                const bool incrementalMerge = annotationsDeltaData.isValid();

                if (targetDelta->hasType(ProjectTimelineDeltas::annotationsAdded))
                {
                    annotationsDeltaData = mergeAnnotationsAdded(
                        incrementalMerge ? annotationsDeltaData : stateDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::annotationsRemoved))
                {
                    annotationsDeltaData = mergeAnnotationsRemoved(
                        incrementalMerge ? annotationsDeltaData : stateDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::annotationsChanged))
                {
                    annotationsDeltaData = mergeAnnotationsChanged(
                        incrementalMerge ? annotationsDeltaData : stateDeltaData, targetDeltaData);
                }
            }
            else if (bothDeltasAreTimeSignatureType)
            {
                deltaFoundInChanges = true;
                const bool incrementalMerge = timeSignaturesDeltaData.isValid();
                
                if (targetDelta->hasType(ProjectTimelineDeltas::timeSignaturesAdded))
                {
                    timeSignaturesDeltaData = mergeTimeSignaturesAdded(
                        incrementalMerge ? timeSignaturesDeltaData : stateDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::timeSignaturesRemoved))
                {
                    timeSignaturesDeltaData = mergeTimeSignaturesRemoved(
                        incrementalMerge ? timeSignaturesDeltaData : stateDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::timeSignaturesChanged))
                {
                    timeSignaturesDeltaData = mergeTimeSignaturesChanged(
                        incrementalMerge ? timeSignaturesDeltaData : stateDeltaData, targetDeltaData);
                }
            }
            else if (bothDeltasAreKeySignatureType)
            {
                deltaFoundInChanges = true;
                const bool incrementalMerge = keySignaturesDeltaData.isValid();

                if (targetDelta->hasType(ProjectTimelineDeltas::keySignaturesAdded))
                {
                    keySignaturesDeltaData = mergeKeySignaturesAdded(
                        incrementalMerge ? keySignaturesDeltaData : stateDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::keySignaturesRemoved))
                {
                    keySignaturesDeltaData = mergeKeySignaturesRemoved(
                        incrementalMerge ? keySignaturesDeltaData : stateDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::keySignaturesChanged))
                {
                    keySignaturesDeltaData = mergeKeySignaturesChanged(
                        incrementalMerge ? keySignaturesDeltaData : stateDeltaData, targetDeltaData);
                }
            }
        }

        if (annotationsDeltaData.isValid())
        {
            diff->applyDelta(annotationsDelta.release(), annotationsDeltaData);
        }
        
        if (timeSignaturesDeltaData.isValid())
        {
            diff->applyDelta(timeSignaturesDelta.release(), timeSignaturesDeltaData);
        }
        
        if (keySignaturesDeltaData.isValid())
        {
            diff->applyDelta(keySignaturesDelta.release(), keySignaturesDeltaData);
        }

        if (! deltaFoundInChanges)
        {
            diff->applyDelta(stateDelta->createCopy(), stateDeltaData);
        }
    }

    // step 2:
    // resolve new delta types that may be missing in project history state,
    // e.g., a project that was created with earlier versions of the app,
    // which has a history tree with timeline initialised with annotation deltas
    // and time signature deltas, but missing key signature deltas (introduced later)

    bool stateHasAnnotations = false;
    bool stateHasTimeSignatures = false;
    bool stateHasKeySignatures = false;

    for (int i = 0; i < initialState.getNumDeltas(); ++i)
    {
        const Delta *stateDelta = initialState.getDelta(i);
        stateHasAnnotations = stateHasAnnotations || checkIfDeltaIsAnnotationType(stateDelta);
        stateHasTimeSignatures = stateHasTimeSignatures || checkIfDeltaIsTimeSignatureType(stateDelta);
        stateHasKeySignatures = stateHasKeySignatures || checkIfDeltaIsKeySignatureType(stateDelta);
    }

    for (int i = 0; i < initialState.getNumDeltas(); ++i)
    {
        UniquePointer<Delta> annotationsDelta(
            new Delta(DeltaDescription(Serialization::VCS::headStateDelta),
                ProjectTimelineDeltas::annotationsAdded));

        UniquePointer<Delta> keySignaturesDelta(
            new Delta(DeltaDescription(Serialization::VCS::headStateDelta),
                ProjectTimelineDeltas::keySignaturesAdded));

        UniquePointer<Delta> timeSignaturesDelta(
            new Delta(DeltaDescription(Serialization::VCS::headStateDelta),
                ProjectTimelineDeltas::timeSignaturesAdded));

        SerializedData annotationsDeltaData;
        SerializedData keySignaturesDeltaData;
        SerializedData timeSignaturesDeltaData;

        for (int j = 0; j < this->target.getNumDeltas(); ++j)
        {
            const Delta *targetDelta = this->target.getDelta(j);
            const auto targetDeltaData(this->target.getDeltaData(j));

            const bool foundMissingKeySignature = !stateHasKeySignatures && checkIfDeltaIsKeySignatureType(targetDelta);
            const bool foundMissingTimeSignature = !stateHasTimeSignatures && checkIfDeltaIsTimeSignatureType(targetDelta);
            const bool foundMissingAnnotation = !stateHasAnnotations && checkIfDeltaIsAnnotationType(targetDelta);

            if (foundMissingKeySignature)
            {
                const bool incrementalMerge = keySignaturesDeltaData.isValid();
                SerializedData emptyKeySignaturesDeltaData(serializeTimelineSequence({}, ProjectTimelineDeltas::keySignaturesAdded));

                if (targetDelta->hasType(ProjectTimelineDeltas::keySignaturesAdded))
                {
                    keySignaturesDeltaData = mergeKeySignaturesAdded(
                        incrementalMerge ? keySignaturesDeltaData : emptyKeySignaturesDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::keySignaturesRemoved))
                {
                    keySignaturesDeltaData = mergeKeySignaturesRemoved(
                        incrementalMerge ? keySignaturesDeltaData : emptyKeySignaturesDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::keySignaturesChanged))
                {
                    keySignaturesDeltaData = mergeKeySignaturesChanged(
                        incrementalMerge ? keySignaturesDeltaData : emptyKeySignaturesDeltaData, targetDeltaData);
                }
            }
            else if (foundMissingTimeSignature)
            {
                const bool incrementalMerge = timeSignaturesDeltaData.isValid();
                SerializedData emptyTimeSignaturesDeltaData(serializeTimelineSequence({}, ProjectTimelineDeltas::timeSignaturesAdded));

                if (targetDelta->hasType(ProjectTimelineDeltas::timeSignaturesAdded))
                {
                    timeSignaturesDeltaData = mergeTimeSignaturesAdded(
                        incrementalMerge ? timeSignaturesDeltaData : emptyTimeSignaturesDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::timeSignaturesRemoved))
                {
                    timeSignaturesDeltaData = mergeTimeSignaturesRemoved(
                        incrementalMerge ? timeSignaturesDeltaData : emptyTimeSignaturesDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::timeSignaturesChanged))
                {
                    timeSignaturesDeltaData = mergeTimeSignaturesChanged(
                        incrementalMerge ? timeSignaturesDeltaData : emptyTimeSignaturesDeltaData, targetDeltaData);
                }
            }
            else if (foundMissingAnnotation)
            {
                const bool incrementalMerge = annotationsDeltaData.isValid();
                SerializedData emptyAnnotationDeltaData(serializeTimelineSequence({}, ProjectTimelineDeltas::annotationsAdded));

                if (targetDelta->hasType(ProjectTimelineDeltas::annotationsAdded))
                {
                    annotationsDeltaData = mergeAnnotationsAdded(
                        incrementalMerge ? annotationsDeltaData : emptyAnnotationDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::annotationsRemoved))
                {
                    annotationsDeltaData = mergeAnnotationsRemoved(
                        incrementalMerge ? annotationsDeltaData : emptyAnnotationDeltaData, targetDeltaData);
                }
                else if (targetDelta->hasType(ProjectTimelineDeltas::annotationsChanged))
                {
                    annotationsDeltaData = mergeAnnotationsChanged(
                        incrementalMerge ? annotationsDeltaData : emptyAnnotationDeltaData, targetDeltaData);
                }
            }
        }

        if (annotationsDeltaData.isValid())
        {
            diff->applyDelta(annotationsDelta.release(), annotationsDeltaData);
        }

        if (timeSignaturesDeltaData.isValid())
        {
            diff->applyDelta(timeSignaturesDelta.release(), timeSignaturesDeltaData);
        }

        if (keySignaturesDeltaData.isValid())
        {
            diff->applyDelta(keySignaturesDelta.release(), keySignaturesDeltaData);
        }
    }

    return diff;
}

//===----------------------------------------------------------------------===//
// Merge annotations
//===----------------------------------------------------------------------===//

SerializedData mergeAnnotationsAdded(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<const MidiEvent *> result;

    result.addArray(stateEvents);

    // check if state doesn't already have events with the same ids, then add
    for (int i = 0; i < changesEvents.size(); ++i)
    {
        bool foundEventInState = false;
        const AnnotationEvent *changesEvent =
            static_cast<AnnotationEvent *>(changesEvents.getUnchecked(i));

        for (int j = 0; j < stateEvents.size(); ++j)
        {
            const AnnotationEvent *stateEvent =
                static_cast<AnnotationEvent *>(stateEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInState = true;
                break;
            }
        }

        if (! foundEventInState)
        {
            result.add(changesEvent);
        }
    }

    return serializeTimelineSequence(result, ProjectTimelineDeltas::annotationsAdded);
}

SerializedData mergeAnnotationsRemoved(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<const MidiEvent *> result;

    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const AnnotationEvent *stateEvent =
            static_cast<AnnotationEvent *>(stateEvents.getUnchecked(i));

        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const AnnotationEvent *changesEvent =
                static_cast<AnnotationEvent *>(changesEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                break;
            }
        }

        if (! foundEventInChanges)
        {
            result.add(stateEvent);
        }
    }

    return serializeTimelineSequence(result, ProjectTimelineDeltas::annotationsAdded);
}

SerializedData mergeAnnotationsChanged(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<const MidiEvent *> result;

    result.addArray(stateEvents);

    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const AnnotationEvent *stateEvent =
            static_cast<AnnotationEvent *>(stateEvents.getUnchecked(i));

        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const AnnotationEvent *changesEvent =
                static_cast<AnnotationEvent *>(changesEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                result.removeAllInstancesOf(stateEvent);
                result.addIfNotAlreadyThere(changesEvent);

                break;
            }
        }

        //jassert(foundEventInChanges);
    }

    return serializeTimelineSequence(result, ProjectTimelineDeltas::annotationsAdded);
}

//===----------------------------------------------------------------------===//
// Merge time signatures
//===----------------------------------------------------------------------===//

SerializedData mergeTimeSignaturesAdded(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);
    
    Array<const MidiEvent *> result;
    
    result.addArray(stateEvents);
    
    // check if state doesn't already have events with the same ids, then add
    for (int i = 0; i < changesEvents.size(); ++i)
    {
        bool foundEventInState = false;
        const TimeSignatureEvent *changesEvent =
            static_cast<TimeSignatureEvent *>(changesEvents.getUnchecked(i));
        
        for (int j = 0; j < stateEvents.size(); ++j)
        {
            const TimeSignatureEvent *stateEvent =
                static_cast<TimeSignatureEvent *>(stateEvents.getUnchecked(j));
            
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInState = true;
                break;
            }
        }
        
        if (! foundEventInState)
        {
            result.add(changesEvent);
        }
    }
    
    return serializeTimelineSequence(result, ProjectTimelineDeltas::timeSignaturesAdded);
}

SerializedData mergeTimeSignaturesRemoved(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);
    
    Array<const MidiEvent *> result;
    
    // add all events that are missing in changes
    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const TimeSignatureEvent *stateEvent =
            static_cast<TimeSignatureEvent *>(stateEvents.getUnchecked(i));
        
        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const TimeSignatureEvent *changesEvent =
                static_cast<TimeSignatureEvent *>(changesEvents.getUnchecked(j));
            
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                break;
            }
        }
        
        if (! foundEventInChanges)
        {
            result.add(stateEvent);
        }
    }
    
    return serializeTimelineSequence(result, ProjectTimelineDeltas::timeSignaturesAdded);
}

SerializedData mergeTimeSignaturesChanged(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);
    
    Array<const MidiEvent *> result;
    result.addArray(stateEvents);
    
    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const TimeSignatureEvent *stateEvent =
            static_cast<TimeSignatureEvent *>(stateEvents.getUnchecked(i));
        
        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const TimeSignatureEvent *changesEvent =
                static_cast<TimeSignatureEvent *>(changesEvents.getUnchecked(j));
            
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                result.removeAllInstancesOf(stateEvent);
                result.addIfNotAlreadyThere(changesEvent);
                
                break;
            }
        }
        
        //jassert(foundEventInChanges);
    }
    
    return serializeTimelineSequence(result, ProjectTimelineDeltas::timeSignaturesAdded);
}

//===----------------------------------------------------------------------===//
// Merge key signatures
//===----------------------------------------------------------------------===//

SerializedData mergeKeySignaturesAdded(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<const MidiEvent *> result;

    result.addArray(stateEvents);

    // check if state doesn't already have events with the same ids, then add
    for (int i = 0; i < changesEvents.size(); ++i)
    {
        bool foundEventInState = false;
        const KeySignatureEvent *changesEvent =
            static_cast<KeySignatureEvent *>(changesEvents.getUnchecked(i));

        for (int j = 0; j < stateEvents.size(); ++j)
        {
            const KeySignatureEvent *stateEvent =
                static_cast<KeySignatureEvent *>(stateEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInState = true;
                break;
            }
        }

        if (!foundEventInState)
        {
            result.add(changesEvent);
        }
    }

    return serializeTimelineSequence(result, ProjectTimelineDeltas::keySignaturesAdded);
}

SerializedData mergeKeySignaturesRemoved(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<const MidiEvent *> result;

    // add all events that are missing in changes
    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const KeySignatureEvent *stateEvent =
            static_cast<KeySignatureEvent *>(stateEvents.getUnchecked(i));

        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const KeySignatureEvent *changesEvent =
                static_cast<KeySignatureEvent *>(changesEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                break;
            }
        }

        if (!foundEventInChanges)
        {
            result.add(stateEvent);
        }
    }

    return serializeTimelineSequence(result, ProjectTimelineDeltas::keySignaturesAdded);
}

SerializedData mergeKeySignaturesChanged(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<const MidiEvent *> result;
    result.addArray(stateEvents);

    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const KeySignatureEvent *stateEvent =
            static_cast<KeySignatureEvent *>(stateEvents.getUnchecked(i));

        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const KeySignatureEvent *changesEvent =
                static_cast<KeySignatureEvent *>(changesEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                result.removeAllInstancesOf(stateEvent);
                result.addIfNotAlreadyThere(changesEvent);

                break;
            }
        }

        //jassert(foundEventInChanges);
    }

    return serializeTimelineSequence(result, ProjectTimelineDeltas::keySignaturesAdded);
}


//===----------------------------------------------------------------------===//
// Diff
//===----------------------------------------------------------------------===//

Array<DeltaDiff> createAnnotationsDiffs(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<DeltaDiff> res;
    Array<const MidiEvent *> addedEvents;
    Array<const MidiEvent *> removedEvents;
    Array<const MidiEvent *> changedEvents;

    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const AnnotationEvent *stateEvent =
            static_cast<AnnotationEvent *>(stateEvents.getUnchecked(i));

        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const AnnotationEvent *changesEvent =
                static_cast<AnnotationEvent *>(changesEvents.getUnchecked(j));

            // state event was found in changes, add `changed` records
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;

                const bool eventHasChanged =
                    (stateEvent->getBeat() != changesEvent->getBeat() ||
                     stateEvent->getLength() != changesEvent->getLength() ||
                     stateEvent->getColour() != changesEvent->getColour() ||
                     stateEvent->getDescription() != changesEvent->getDescription());

                if (eventHasChanged)
                {
                    changedEvents.add(changesEvent);
                }

                break;
            }
        }

        // state event was not found in changes, add `removed` record
        if (! foundEventInChanges)
        {
            removedEvents.add(stateEvent);
        }
    }

    // search for the new events missing in state
    for (int i = 0; i < changesEvents.size(); ++i)
    {
        bool foundEventInState = false;
        const AnnotationEvent *changesEvent =
            static_cast<AnnotationEvent *>(changesEvents.getUnchecked(i));

        for (int j = 0; j < stateEvents.size(); ++j)
        {
            const AnnotationEvent *stateEvent =
                static_cast<AnnotationEvent *>(stateEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInState = true;
                break;
            }
        }

        if (! foundEventInState)
        {
            addedEvents.add(changesEvent);
        }
    }

    // serialize deltas, if any
    if (addedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(addedEvents,
            "added {x} annotations",
            addedEvents.size(),
            ProjectTimelineDeltas::annotationsAdded));
    }

    if (removedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(removedEvents,
            "removed {x} annotations",
            removedEvents.size(),
            ProjectTimelineDeltas::annotationsRemoved));
    }

    if (changedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(changedEvents,
            "changed {x} annotations",
            changedEvents.size(),
            ProjectTimelineDeltas::annotationsChanged));
    }

    return res;
}

Array<DeltaDiff> createTimeSignaturesDiffs(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);
    
    Array<DeltaDiff> res;
    Array<const MidiEvent *> addedEvents;
    Array<const MidiEvent *> removedEvents;
    Array<const MidiEvent *> changedEvents;
    
    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const TimeSignatureEvent *stateEvent =
            static_cast<TimeSignatureEvent *>(stateEvents.getUnchecked(i));
        
        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const TimeSignatureEvent *changesEvent =
                static_cast<TimeSignatureEvent *>(changesEvents.getUnchecked(j));
            
            // state event was found in changes, add `changed` records
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;
                
                const bool eventHasChanged =
                    (stateEvent->getBeat() != changesEvent->getBeat() ||
                     stateEvent->getNumerator() != changesEvent->getNumerator() ||
                     stateEvent->getDenominator() != changesEvent->getDenominator());
                
                if (eventHasChanged)
                {
                    changedEvents.add(changesEvent);
                }
                
                break;
            }
        }
        
        // state event was not found in changes, add `removed` record
        if (! foundEventInChanges)
        {
            removedEvents.add(stateEvent);
        }
    }
    
    // search for the new events missing in state
    for (int i = 0; i < changesEvents.size(); ++i)
    {
        bool foundEventInState = false;
        const TimeSignatureEvent *changesEvent =
            static_cast<TimeSignatureEvent *>(changesEvents.getUnchecked(i));
        
        for (int j = 0; j < stateEvents.size(); ++j)
        {
            const TimeSignatureEvent *stateEvent =
                static_cast<TimeSignatureEvent *>(stateEvents.getUnchecked(j));
            
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInState = true;
                break;
            }
        }
        
        if (! foundEventInState)
        {
            addedEvents.add(changesEvent);
        }
    }
    
    // serialize deltas, if any
    if (addedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(addedEvents,
            "added {x} time signatures",
            addedEvents.size(),
            ProjectTimelineDeltas::timeSignaturesAdded));
    }
    
    if (removedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(removedEvents,
            "removed {x} time signatures",
            removedEvents.size(),
            ProjectTimelineDeltas::timeSignaturesRemoved));
    }
    
    if (changedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(changedEvents,
            "changed {x} time signatures",
            changedEvents.size(),
            ProjectTimelineDeltas::timeSignaturesChanged));
    }
    
    return res;
}

Array<DeltaDiff> createKeySignaturesDiffs(const SerializedData &state, const SerializedData &changes)
{
    using namespace Serialization::VCS;

    OwnedArray<MidiEvent> stateEvents;
    OwnedArray<MidiEvent> changesEvents;
    deserializeTimelineChanges(state, changes, stateEvents, changesEvents);

    Array<DeltaDiff> res;
    Array<const MidiEvent *> addedEvents;
    Array<const MidiEvent *> removedEvents;
    Array<const MidiEvent *> changedEvents;

    for (int i = 0; i < stateEvents.size(); ++i)
    {
        bool foundEventInChanges = false;
        const KeySignatureEvent *stateEvent =
            static_cast<KeySignatureEvent *>(stateEvents.getUnchecked(i));

        for (int j = 0; j < changesEvents.size(); ++j)
        {
            const KeySignatureEvent *changesEvent =
                static_cast<KeySignatureEvent *>(changesEvents.getUnchecked(j));

            // state event was found in changes, add `changed` records
            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInChanges = true;

                const bool eventHasChanged =
                    (stateEvent->getBeat() != changesEvent->getBeat() ||
                        stateEvent->getRootKey() != changesEvent->getRootKey() ||
                        ! stateEvent->getScale()->isEquivalentTo(changesEvent->getScale()));

                if (eventHasChanged)
                {
                    changedEvents.add(changesEvent);
                }

                break;
            }
        }

        // state event was not found in changes, add `removed` record
        if (!foundEventInChanges)
        {
            removedEvents.add(stateEvent);
        }
    }

    // search for the new events missing in state
    for (int i = 0; i < changesEvents.size(); ++i)
    {
        bool foundEventInState = false;
        const KeySignatureEvent *changesEvent =
            static_cast<KeySignatureEvent *>(changesEvents.getUnchecked(i));

        for (int j = 0; j < stateEvents.size(); ++j)
        {
            const KeySignatureEvent *stateEvent =
                static_cast<KeySignatureEvent *>(stateEvents.getUnchecked(j));

            if (stateEvent->getId() == changesEvent->getId())
            {
                foundEventInState = true;
                break;
            }
        }

        if (!foundEventInState)
        {
            addedEvents.add(changesEvent);
        }
    }

    // serialize deltas, if any
    if (addedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(addedEvents,
            "added {x} key signatures",
            addedEvents.size(),
            ProjectTimelineDeltas::keySignaturesAdded));
    }

    if (removedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(removedEvents,
            "removed {x} key signatures",
            removedEvents.size(),
            ProjectTimelineDeltas::keySignaturesRemoved));
    }

    if (changedEvents.size() > 0)
    {
        res.add(serializeTimelineChanges(changedEvents,
            "changed {x} key signatures",
            changedEvents.size(),
            ProjectTimelineDeltas::keySignaturesChanged));
    }

    return res;
}


//===----------------------------------------------------------------------===//
// Serialization
//===----------------------------------------------------------------------===//

void deserializeTimelineChanges(const SerializedData &state, const SerializedData &changes,
    OwnedArray<MidiEvent> &stateEvents, OwnedArray<MidiEvent> &changesEvents)
{
    using namespace Serialization;

    if (state.isValid())
    {
        forEachChildWithType(state, e, Midi::annotation)
        {
            AnnotationEvent *event = new AnnotationEvent();
            event->deserialize(e);
            stateEvents.addSorted(*event, event);
        }

        forEachChildWithType(state, e, Midi::timeSignature)
        {
            TimeSignatureEvent *event = new TimeSignatureEvent();
            event->deserialize(e);
            stateEvents.addSorted(*event, event);
        }

        forEachChildWithType(state, e, Midi::keySignature)
        {
            KeySignatureEvent *event = new KeySignatureEvent();
            event->deserialize(e);
            stateEvents.addSorted(*event, event);
        }
    }

    if (changes.isValid())
    {
        forEachChildWithType(changes, e, Midi::annotation)
        {
            AnnotationEvent *event = new AnnotationEvent();
            event->deserialize(e);
            changesEvents.addSorted(*event, event);
        }
        
        forEachChildWithType(changes, e, Midi::timeSignature)
        {
            TimeSignatureEvent *event = new TimeSignatureEvent();
            event->deserialize(e);
            changesEvents.addSorted(*event, event);
        }

        forEachChildWithType(changes, e, Midi::keySignature)
        {
            KeySignatureEvent *event = new KeySignatureEvent();
            event->deserialize(e);
            changesEvents.addSorted(*event, event);
        }
    }
}

DeltaDiff serializeTimelineChanges(Array<const MidiEvent *> changes,
        const String &description, int64 numChanges, const Identifier &deltaType)
{
    DeltaDiff changesFullDelta;
    changesFullDelta.delta.reset(new Delta(DeltaDescription(description, numChanges), deltaType));
    changesFullDelta.deltaData = serializeTimelineSequence(changes, deltaType);
    return changesFullDelta;
}

SerializedData serializeTimelineSequence(Array<const MidiEvent *> changes, const Identifier &tag)
{
    SerializedData tree(tag);

    for (int i = 0; i < changes.size(); ++i)
    {
        const MidiEvent *event = changes.getUnchecked(i);
        tree.appendChild(event->serialize());
    }

    return tree;
}

bool checkIfDeltaIsAnnotationType(const Delta *d)
{
    using namespace Serialization::VCS;
    return (d->hasType(ProjectTimelineDeltas::annotationsAdded) ||
        d->hasType(ProjectTimelineDeltas::annotationsChanged) ||
        d->hasType(ProjectTimelineDeltas::annotationsRemoved));
}

bool checkIfDeltaIsTimeSignatureType(const Delta *d)
{
    using namespace Serialization::VCS;
    return (d->hasType(ProjectTimelineDeltas::timeSignaturesAdded) ||
        d->hasType(ProjectTimelineDeltas::timeSignaturesChanged) ||
        d->hasType(ProjectTimelineDeltas::timeSignaturesRemoved));
}

bool checkIfDeltaIsKeySignatureType(const Delta *d)
{
    using namespace Serialization::VCS;
    return (d->hasType(ProjectTimelineDeltas::keySignaturesAdded) ||
        d->hasType(ProjectTimelineDeltas::keySignaturesRemoved) ||
        d->hasType(ProjectTimelineDeltas::keySignaturesChanged));
}

}
