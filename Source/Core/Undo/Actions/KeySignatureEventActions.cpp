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
#include "KeySignatureEventActions.h"
#include "KeySignaturesSequence.h"
#include "MidiTrackSource.h"
#include "SerializationKeys.h"

//===----------------------------------------------------------------------===//
// Insert
//===----------------------------------------------------------------------===//

KeySignatureEventInsertAction::KeySignatureEventInsertAction(MidiTrackSource &source,
    const String &trackId, const KeySignatureEvent &event) noexcept :
    UndoAction(source),
    trackId(trackId),
    event(event) {}

bool KeySignatureEventInsertAction::perform()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return (sequence->insert(this->event, false) != nullptr);
    }
    
    return false;
}

bool KeySignatureEventInsertAction::undo()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->remove(this->event, false);
    }
    
    return false;
}

int KeySignatureEventInsertAction::getSizeInUnits()
{
    return sizeof(KeySignatureEvent);
}

SerializedData KeySignatureEventInsertAction::serialize() const
{
    SerializedData tree(Serialization::Undo::keySignatureEventInsertAction);
    tree.setProperty(Serialization::Undo::trackId, this->trackId);
    tree.appendChild(this->event.serialize());
    return tree;
}

void KeySignatureEventInsertAction::deserialize(const SerializedData &data)
{
    this->trackId = data.getProperty(Serialization::Undo::trackId);
    this->event.deserialize(data.getChild(0));
}

void KeySignatureEventInsertAction::reset()
{
    this->event.reset();
    this->trackId.clear();
}

//===----------------------------------------------------------------------===//
// Remove
//===----------------------------------------------------------------------===//

KeySignatureEventRemoveAction::KeySignatureEventRemoveAction(MidiTrackSource &source,
    const String &trackId, const KeySignatureEvent &target) noexcept :
    UndoAction(source),
    trackId(trackId),
    event(target) {}

bool KeySignatureEventRemoveAction::perform()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->remove(this->event, false);
    }
    
    return false;
}

bool KeySignatureEventRemoveAction::undo()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return (sequence->insert(this->event, false) != nullptr);
    }
    
    return false;
}

int KeySignatureEventRemoveAction::getSizeInUnits()
{
    return sizeof(KeySignatureEvent);
}

SerializedData KeySignatureEventRemoveAction::serialize() const
{
    SerializedData tree(Serialization::Undo::keySignatureEventRemoveAction);
    tree.setProperty(Serialization::Undo::trackId, this->trackId);
    tree.appendChild(this->event.serialize());
    return tree;
}

void KeySignatureEventRemoveAction::deserialize(const SerializedData &data)
{
    this->trackId = data.getProperty(Serialization::Undo::trackId);
    this->event.deserialize(data.getChild(0));
}

void KeySignatureEventRemoveAction::reset()
{
    this->event.reset();
    this->trackId.clear();
}

//===----------------------------------------------------------------------===//
// Change
//===----------------------------------------------------------------------===//

KeySignatureEventChangeAction::KeySignatureEventChangeAction(MidiTrackSource &source,
    const String &trackId, const KeySignatureEvent &target,
    const KeySignatureEvent &newParameters) noexcept :
    UndoAction(source),
    trackId(trackId),
    eventBefore(target),
    eventAfter(newParameters) {}

bool KeySignatureEventChangeAction::perform()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->change(this->eventBefore, this->eventAfter, false);
    }
    
    return false;
}

bool KeySignatureEventChangeAction::undo()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->change(this->eventAfter, this->eventBefore, false);
    }
    
    return false;
}

int KeySignatureEventChangeAction::getSizeInUnits()
{
    return sizeof(KeySignatureEvent) * 2;
}

UndoAction *KeySignatureEventChangeAction::createCoalescedAction(UndoAction *nextAction)
{
    if (auto *nextChanger = dynamic_cast<KeySignatureEventChangeAction *>(nextAction))
    {
        const bool idsAreEqual =
            (this->eventBefore.getId() == nextChanger->eventAfter.getId() &&
                this->trackId == nextChanger->trackId);

        if (idsAreEqual)
        {
            return new KeySignatureEventChangeAction(this->source,
                this->trackId, this->eventBefore, nextChanger->eventAfter);
        }
    }

    (void) nextAction;
    return nullptr;
}

SerializedData KeySignatureEventChangeAction::serialize() const
{
    SerializedData tree(Serialization::Undo::keySignatureEventChangeAction);
    tree.setProperty(Serialization::Undo::trackId, this->trackId);
    
    SerializedData keySignatureBeforeChild(Serialization::Undo::keySignatureBefore);
    keySignatureBeforeChild.appendChild(this->eventBefore.serialize());
    tree.appendChild(keySignatureBeforeChild);
    
    SerializedData keySignatureAfterChild(Serialization::Undo::keySignatureAfter);
    keySignatureAfterChild.appendChild(this->eventAfter.serialize());
    tree.appendChild(keySignatureAfterChild);
    
    return tree;
}

void KeySignatureEventChangeAction::deserialize(const SerializedData &data)
{
    this->trackId = data.getProperty(Serialization::Undo::trackId);
    
    const auto keySignatureBeforeChild = data.getChildWithName(Serialization::Undo::keySignatureBefore);
    const auto keySignatureAfterChild = data.getChildWithName(Serialization::Undo::keySignatureAfter);
    
    this->eventBefore.deserialize(keySignatureBeforeChild.getChild(0));
    this->eventAfter.deserialize(keySignatureAfterChild.getChild(0));
}

void KeySignatureEventChangeAction::reset()
{
    this->eventBefore.reset();
    this->eventAfter.reset();
    this->trackId.clear();
}

//===----------------------------------------------------------------------===//
// Insert Group
//===----------------------------------------------------------------------===//

KeySignatureEventsGroupInsertAction::KeySignatureEventsGroupInsertAction(MidiTrackSource &source,
    const String &trackId, Array<KeySignatureEvent> &target) noexcept :
    UndoAction(source),
    trackId(trackId)
{
    this->signatures.swapWith(target);
}

bool KeySignatureEventsGroupInsertAction::perform()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->insertGroup(this->signatures, false);
    }
    
    return false;
}

bool KeySignatureEventsGroupInsertAction::undo()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->removeGroup(this->signatures, false);
    }
    
    return false;
}

int KeySignatureEventsGroupInsertAction::getSizeInUnits()
{
    return (sizeof(KeySignatureEvent) * this->signatures.size());
}

SerializedData KeySignatureEventsGroupInsertAction::serialize() const
{
    SerializedData tree(Serialization::Undo::keySignatureEventsGroupInsertAction);
    tree.setProperty(Serialization::Undo::trackId, this->trackId);
    
    for (int i = 0; i < this->signatures.size(); ++i)
    {
        tree.appendChild(this->signatures.getUnchecked(i).serialize());
    }
    
    return tree;
}

void KeySignatureEventsGroupInsertAction::deserialize(const SerializedData &data)
{
    this->reset();
    this->trackId = data.getProperty(Serialization::Undo::trackId);
    
    for (const auto &params : data)
    {
        KeySignatureEvent ae;
        ae.deserialize(params);
        this->signatures.add(ae);
    }
}

void KeySignatureEventsGroupInsertAction::reset()
{
    this->signatures.clear();
    this->trackId.clear();
}

//===----------------------------------------------------------------------===//
// Remove Group
//===----------------------------------------------------------------------===//

KeySignatureEventsGroupRemoveAction::KeySignatureEventsGroupRemoveAction(MidiTrackSource &source,
    const String &trackId, Array<KeySignatureEvent> &target) noexcept :
    UndoAction(source),
    trackId(trackId)
{
    this->signatures.swapWith(target);
}

bool KeySignatureEventsGroupRemoveAction::perform()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->removeGroup(this->signatures, false);
    }
    
    return false;
}

bool KeySignatureEventsGroupRemoveAction::undo()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->insertGroup(this->signatures, false);
    }
    
    return false;
}

int KeySignatureEventsGroupRemoveAction::getSizeInUnits()
{
    return (sizeof(KeySignatureEvent) * this->signatures.size());
}

SerializedData KeySignatureEventsGroupRemoveAction::serialize() const
{
    SerializedData tree(Serialization::Undo::keySignatureEventsGroupRemoveAction);
    tree.setProperty(Serialization::Undo::trackId, this->trackId);
    
    for (int i = 0; i < this->signatures.size(); ++i)
    {
        tree.appendChild(this->signatures.getUnchecked(i).serialize());
    }
    
    return tree;
}

void KeySignatureEventsGroupRemoveAction::deserialize(const SerializedData &data)
{
    this->reset();
    this->trackId = data.getProperty(Serialization::Undo::trackId);
    
    for (const auto &params : data)
    {
        KeySignatureEvent ae;
        ae.deserialize(params);
        this->signatures.add(ae);
    }
}

void KeySignatureEventsGroupRemoveAction::reset()
{
    this->signatures.clear();
    this->trackId.clear();
}

//===----------------------------------------------------------------------===//
// Change Group
//===----------------------------------------------------------------------===//

KeySignatureEventsGroupChangeAction::KeySignatureEventsGroupChangeAction(MidiTrackSource &source,
    const String &trackId, const Array<KeySignatureEvent> state1,
    const Array<KeySignatureEvent> state2) noexcept :
    UndoAction(source),
    trackId(trackId)
{
    this->eventsBefore.addArray(state1);
    this->eventsAfter.addArray(state2);
}

bool KeySignatureEventsGroupChangeAction::perform()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->changeGroup(this->eventsBefore, this->eventsAfter, false);
    }
    
    return false;
}

bool KeySignatureEventsGroupChangeAction::undo()
{
    if (KeySignaturesSequence *sequence =
        this->source.findSequenceByTrackId<KeySignaturesSequence>(this->trackId))
    {
        return sequence->changeGroup(this->eventsAfter, this->eventsBefore, false);
    }
    
    return false;
}

int KeySignatureEventsGroupChangeAction::getSizeInUnits()
{
    return (sizeof(KeySignatureEvent) * this->eventsBefore.size()) +
           (sizeof(KeySignatureEvent) * this->eventsAfter.size());
}

UndoAction *KeySignatureEventsGroupChangeAction::createCoalescedAction(UndoAction *nextAction)
{
    if (auto *nextChanger = dynamic_cast<KeySignatureEventsGroupChangeAction *>(nextAction))
    {
        if (nextChanger->trackId != this->trackId)
        {
            return nullptr;
        }

        bool arraysContainSameEvents =
            (this->eventsBefore.size() == nextChanger->eventsAfter.size()) &&
            (this->eventsBefore[0].getId() == nextChanger->eventsAfter[0].getId());

        if (arraysContainSameEvents)
        {
            return new KeySignatureEventsGroupChangeAction(this->source,
                this->trackId, this->eventsBefore, nextChanger->eventsAfter);
        }
    }

    (void) nextAction;
    return nullptr;
}

//===----------------------------------------------------------------------===//
// Serializable
//===----------------------------------------------------------------------===//

SerializedData KeySignatureEventsGroupChangeAction::serialize() const
{
    SerializedData tree(Serialization::Undo::keySignatureEventsGroupChangeAction);
    tree.setProperty(Serialization::Undo::trackId, this->trackId);
    
    SerializedData groupBeforeChild(Serialization::Undo::groupBefore);
    SerializedData groupAfterChild(Serialization::Undo::groupAfter);
    
    for (int i = 0; i < this->eventsBefore.size(); ++i)
    {
        groupBeforeChild.appendChild(this->eventsBefore.getUnchecked(i).serialize());
    }
    
    for (int i = 0; i < this->eventsAfter.size(); ++i)
    {
        groupAfterChild.appendChild(this->eventsAfter.getUnchecked(i).serialize());
    }
    
    tree.appendChild(groupBeforeChild);
    tree.appendChild(groupAfterChild);
    
    return tree;
}

void KeySignatureEventsGroupChangeAction::deserialize(const SerializedData &data)
{
    this->reset();
    this->trackId = data.getProperty(Serialization::Undo::trackId);
    
    const auto groupBeforeChild = data.getChildWithName(Serialization::Undo::groupBefore);
    const auto groupAfterChild = data.getChildWithName(Serialization::Undo::groupAfter);
    
    for (const auto &params : groupBeforeChild)
    {
        KeySignatureEvent ae;
        ae.deserialize(params);
        this->eventsBefore.add(ae);
    }
    
    for (const auto &params : groupAfterChild)
    {
        KeySignatureEvent ae;
        ae.deserialize(params);
        this->eventsAfter.add(ae);
    }
}

void KeySignatureEventsGroupChangeAction::reset()
{
    this->eventsBefore.clear();
    this->eventsAfter.clear();
    this->trackId.clear();
}
