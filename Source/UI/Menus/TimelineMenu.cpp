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
#include "TimelineMenu.h"
#include "ProjectNode.h"
#include "MainLayout.h"
#include "Icons.h"
#include "HybridRoll.h"
#include "AnnotationEvent.h"
#include "TimeSignatureEvent.h"
#include "PianoTrackNode.h"
#include "ProjectTimeline.h"
#include "MidiSequence.h"
#include "ModalDialogInput.h"
#include "CommandIDs.h"

template<typename T>
const T *findSelectedEventOfType(MidiSequence *const sequence, HybridRoll *const roll, float seekBeat)
{
    const T *selectedEvent = nullptr;

    for (int i = 0; i < sequence->size(); ++i)
    {
        if (T *event = dynamic_cast<T *>(sequence->getUnchecked(i)))
        {
            if (fabs(event->getBeat() - seekBeat) < 0.001f)
            {
                selectedEvent = event;
                break;
            }
        }
    }

    return selectedEvent;
}

TimelineMenu::TimelineMenu(ProjectNode &parentProject) :
    project(parentProject)
{
    const AnnotationEvent *selectedAnnotation = nullptr;
    const KeySignatureEvent *selectedKeySignature = nullptr;
    const TimeSignatureEvent *selectedTimeSignature = nullptr;
    const ProjectTimeline *timeline = this->project.getTimeline();

    if (auto *roll = dynamic_cast<HybridRoll *>(this->project.getLastFocusedRoll()))
    {
        const auto seekBeat = this->project.getTransport().getSeekBeat();
        const auto annotationsSequence = timeline->getAnnotations()->getSequence();
        const auto keySignaturesSequence = timeline->getKeySignatures()->getSequence();
        const auto timeSignaturesSequence = timeline->getTimeSignatures()->getSequence();
        selectedAnnotation = findSelectedEventOfType<AnnotationEvent>(annotationsSequence, roll, seekBeat);
        selectedKeySignature = findSelectedEventOfType<KeySignatureEvent>(keySignaturesSequence, roll, seekBeat);
        selectedTimeSignature = findSelectedEventOfType<TimeSignatureEvent>(timeSignaturesSequence, roll, seekBeat);
    }

    MenuPanel::Menu menu;

    if (selectedAnnotation == nullptr)
    {
        menu.add(MenuItem::item(Icons::create,
            CommandIDs::AddAnnotation,
            TRANS(I18n::Menu::annotationAdd))->closesMenu());
    }

    if (selectedKeySignature == nullptr)
    {
        menu.add(MenuItem::item(Icons::create,
            CommandIDs::AddKeySignature,
            TRANS(I18n::Menu::keySignatureAdd))->closesMenu());
    }

    if (selectedTimeSignature == nullptr)
    {
        menu.add(MenuItem::item(Icons::create,
            CommandIDs::AddTimeSignature,
            TRANS(I18n::Menu::timeSignatureAdd))->closesMenu());
    }

    if (auto *roll = dynamic_cast<HybridRoll *>(this->project.getLastFocusedRoll()))
    {
        const auto annotationsSequence = timeline->getAnnotations()->getSequence();

        for (int i = 0; i < annotationsSequence->size(); ++i)
        {
            if (auto *annotation = dynamic_cast<AnnotationEvent *>(annotationsSequence->getUnchecked(i)))
            {
                //double outTimeMs = 0.0;
                //double outTempo = 0.0;
                //const double seekPos = roll->getTransportPositionByBeat(annotation->getBeat());
                //this->project.getTransport().calcTimeAndTempoAt(seekPos, outTimeMs, outTempo);
                
                menu.add(MenuItem::item(Icons::annotation,
                    annotation->getDescription())->
                    //withSubLabel(Transport::getTimeString(outTimeMs))->
                    colouredWith(annotation->getTrackColour())->
                    closesMenu()->
                    withAction([this, i]()
                    {
                        const auto timeline = this->project.getTimeline();
                        const auto annotations = timeline->getAnnotations()->getSequence();
                        if (auto roll = dynamic_cast<HybridRoll *>(this->project.getLastFocusedRoll()))
                        {
                            if (auto annotation = dynamic_cast<AnnotationEvent *>(annotations->getUnchecked(i)))
                            {
                                this->project.getTransport().seekToBeat(annotation->getBeat());
                                roll->scrollToSeekPosition();
                            }
                        }
                    }));
            }
        }
    }
    else
    {
        jassertfalse;
    }
    
    this->updateContent(menu, SlideDown);
}
