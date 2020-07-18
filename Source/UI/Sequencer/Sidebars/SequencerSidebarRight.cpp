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

//[Headers]
#include "Common.h"
//[/Headers]

#include "SequencerSidebarRight.h"

//[MiscUserDefs]
#include "Transport.h"
#include "ProjectNode.h"
#include "PlayerThread.h"
#include "Icons.h"
#include "HybridRoll.h"
#include "PianoRoll.h"
#include "MidiSequence.h"
#include "MenuItemComponent.h"
#include "ProjectTimeline.h"
#include "HelioCallout.h"
#include "SequencerOperations.h"
#include "NotesTuningPanel.h"
#include "MenuPanel.h"
#include "MainLayout.h"
#include "Workspace.h"
#include "CachedLabelImage.h"
#include "Config.h"
#include "CommandIDs.h"
#include "ColourIDs.h"

//[/MiscUserDefs]

SequencerSidebarRight::SequencerSidebarRight(ProjectNode &parent)
    : project(parent),
      lastSeekTime(0.0),
      lastTotalTime(0.0),
      timerStartSeekTime(0.0),
      timerStartSystemTime(0.0)
{
    this->listBox.reset(new ListBox());
    this->addAndMakeVisible(listBox.get());

    this->headLine.reset(new SeparatorHorizontalReversed());
    this->addAndMakeVisible(headLine.get());
    this->shadow.reset(new ShadowUpwards(Light));
    this->addAndMakeVisible(shadow.get());
    this->separator.reset(new SeparatorHorizontal());
    this->addAndMakeVisible(separator.get());
    this->totalTime.reset(new Label(String(),
                                     String()));
    this->addAndMakeVisible(totalTime.get());
    this->totalTime->setFont(Font (14.00f, Font::plain));
    totalTime->setJustificationType(Justification::centred);
    totalTime->setEditable(false, false, false);

    this->currentTime.reset(new Label(String(),
                                       String()));
    this->addAndMakeVisible(currentTime.get());
    this->currentTime->setFont(Font (16.00f, Font::plain));
    currentTime->setJustificationType(Justification::centred);
    currentTime->setEditable(false, false, false);

    this->headShadow.reset(new ShadowDownwards(Light));
    this->addAndMakeVisible(headShadow.get());
    this->annotationsButton.reset(new MenuItemComponent(this, nullptr, MenuItem::item(Icons::reprise, CommandIDs::ToggleLoopOverSelection)));
    this->addAndMakeVisible(annotationsButton.get());

    this->transportControl.reset(new TransportControlComponent(nullptr));
    this->addAndMakeVisible(transportControl.get());

    //[UserPreSize]
    this->setOpaque(true);
    this->setPaintingIsUnclipped(true);
    this->setInterceptsMouseClicks(false, true);

    this->listBox->setMultipleSelectionEnabled(false);
    this->listBox->setRowHeight(Globals::UI::sidebarRowHeight);
    this->listBox->setModel(this);

    // This one doesn't change too frequently:
    //this->totalTime->setBufferedToImage(true);
    //this->totalTime->setCachedComponentImage(new CachedLabelImage(*this->totalTime));

    // TODO: remove these and show timings somewhere else
#if !TOOLS_SIDEBAR_SHOWS_TIME
    this->totalTime->setVisible(false);
    this->currentTime->setVisible(false);
#endif

    //[/UserPreSize]

    this->setSize(44, 640);

    //[Constructor]
    MenuPanelUtils::disableKeyboardFocusForAllChildren(this);

    this->setSize(Globals::UI::sidebarWidth, 640);

    this->project.getTransport().addTransportListener(this);
    this->project.getEditMode().addChangeListener(this);
    //[/Constructor]
}

SequencerSidebarRight::~SequencerSidebarRight()
{
    //[Destructor_pre]
    this->project.getEditMode().removeChangeListener(this);
    this->project.getTransport().removeTransportListener(this);
    //[/Destructor_pre]

    listBox = nullptr;
    headLine = nullptr;
    shadow = nullptr;
    separator = nullptr;
    totalTime = nullptr;
    currentTime = nullptr;
    headShadow = nullptr;
    annotationsButton = nullptr;
    transportControl = nullptr;

    //[Destructor]
    //[/Destructor]
}

void SequencerSidebarRight::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    const auto &theme = HelioTheme::getCurrentTheme();
    g.setFillType({ theme.getBgCacheC(), {} });
    g.fillRect(this->getLocalBounds());
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    g.setColour(findDefaultColour(ColourIDs::Common::borderLineLight));
    g.fillRect(0, 0, 1, this->getHeight());
    //[/UserPaint]
}

void SequencerSidebarRight::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    listBox->setBounds(0, 41, getWidth() - 0, getHeight() - 121);
    headLine->setBounds(0, 39, getWidth() - 0, 2);
    shadow->setBounds(0, getHeight() - 79 - 6, getWidth() - 0, 6);
    separator->setBounds(0, getHeight() - 78 - 2, getWidth() - 0, 2);
    totalTime->setBounds((getWidth() / 2) + 80 - (72 / 2), getHeight() - 9 - 18, 72, 18);
    currentTime->setBounds((getWidth() / 2) + 80 - (72 / 2), getHeight() - 26 - 22, 72, 22);
    headShadow->setBounds(0, 40, getWidth() - 0, 6);
    annotationsButton->setBounds((getWidth() / 2) - ((getWidth() - 0) / 2), 0, getWidth() - 0, 39);
    transportControl->setBounds(0, getHeight() - 79, getWidth() - 0, 79);
    //[UserResized] Add your own custom resize handling here..
    // a hack for themes changing
    this->listBox->updateContent();
    this->annotationsButton->resized();
    //[/UserResized]
}


//[MiscUserCode]
void SequencerSidebarRight::recreateMenu()
{
    this->menu.clear();

    const bool defaultMode = this->project.getEditMode().isMode(HybridRollEditMode::defaultMode);
    const bool drawMode = this->project.getEditMode().isMode(HybridRollEditMode::drawMode);
    const bool scissorsMode = this->project.getEditMode().isMode(HybridRollEditMode::knifeMode);

    // Selection tool is useless on the desktop
#if HELIO_MOBILE
    const bool selectionMode = this->project.getEditMode().isMode(HybridRollEditMode::selectionMode);
    this->menu.add(MenuItem::item(Icons::selectionTool, CommandIDs::EditModeSelect)->toggled(selectionMode));
#endif

    this->menu.add(MenuItem::item(Icons::cursorTool, CommandIDs::EditModeDefault)->toggled(defaultMode));
    this->menu.add(MenuItem::item(Icons::drawTool, CommandIDs::EditModeDraw)->toggled(drawMode));

    // Drag tool is useless on the mobile
#if HELIO_DESKTOP
    const bool dragMode = this->project.getEditMode().isMode(HybridRollEditMode::dragMode);
    this->menu.add(MenuItem::item(Icons::dragTool, CommandIDs::EditModePan)->toggled(dragMode));
#endif

    this->menu.add(MenuItem::item(Icons::cutterTool, CommandIDs::EditModeKnife)->toggled(scissorsMode));

    //this->menu.add(MenuItem::item(Icons::record, CommandIDs::ToggleRecording)->toggled(transportIsPaused));

    if (this->menuMode == MenuMode::PianoRollTools)
    {
        this->menu.add(MenuItem::item(Icons::chordBuilder, CommandIDs::ShowChordPanel));

        if (!App::Config().getArpeggiators()->isEmpty())
        {
            this->menu.add(MenuItem::item(Icons::arpeggiate, CommandIDs::ShowArpeggiatorsPanel));
        }

        //this->menu.add(MenuItem::item(Icons::script, CommandIDs::RunScriptTransform));

        this->menu.add(MenuItem::item(Icons::copy, CommandIDs::CopyEvents));
        this->menu.add(MenuItem::item(Icons::paste, CommandIDs::PasteEvents));
    }

#if HELIO_MOBILE
    if (this->menuMode == MenuMode::PianoRollTools)
    {
        this->menu.add(MenuItem::item(Icons::remove, CommandIDs::DeleteEvents));
    }
    else if (this->menuMode == MenuMode::PatternRollTools)
    {
        this->menu.add(MenuItem::item(Icons::remove, CommandIDs::DeleteClips));
    }
#endif

    this->menu.add(MenuItem::item(Icons::undo, CommandIDs::Undo));
    this->menu.add(MenuItem::item(Icons::redo, CommandIDs::Redo));
}

//===----------------------------------------------------------------------===//
// ListBoxModel
//===----------------------------------------------------------------------===//

Component *SequencerSidebarRight::refreshComponentForRow(int rowNumber,
    bool isRowSelected, Component *existingComponentToUpdate)
{
    if (rowNumber >= this->menu.size())
    {
        return existingComponentToUpdate;
    }

    const auto description = this->menu.getUnchecked(rowNumber);
    if (existingComponentToUpdate != nullptr)
    {
        if (auto *row = dynamic_cast<MenuItemComponent *>(existingComponentToUpdate))
        {
            row->setSelected(isRowSelected);
            row->update(description);
        }
    }
    else
    {
        auto *row = new MenuItemComponent(this, this->listBox->getViewport(), description);
        row->setSelected(isRowSelected);
        return row;
    }

    return existingComponentToUpdate;
}

int SequencerSidebarRight::getNumRows()
{
    return this->menu.size();
}

//===----------------------------------------------------------------------===//
// AsyncUpdater
//===----------------------------------------------------------------------===//

void SequencerSidebarRight::handleAsyncUpdate()
{
#if TOOLS_SIDEBAR_SHOWS_TIME
    if (this->isTimerRunning())
    {
        const double systemTimeOffset = (Time::getMillisecondCounter() - this->timerStartSystemTime.get());
        const double currentTimeMs(this->timerStartSeekTime.get() + systemTimeOffset);
        this->currentTime->setText(Transport::getTimeString(currentTimeMs), dontSendNotification);
    }
    else
    {
        this->currentTime->setText(Transport::getTimeString(this->lastSeekTime.get()), dontSendNotification);
    }

    this->totalTime->setText(Transport::getTimeString(this->lastTotalTime.get()), dontSendNotification);
#endif
}

//===----------------------------------------------------------------------===//
// TransportListener
//===----------------------------------------------------------------------===//

void SequencerSidebarRight::onSeek(float beatPosition, double currentTimeMs, double totalTimeMs)
{
#if TOOLS_SIDEBAR_SHOWS_TIME
    this->lastSeekTime = currentTimeMs;
    this->lastTotalTime = totalTimeMs;
    this->triggerAsyncUpdate();
#endif
}

void SequencerSidebarRight::onTotalTimeChanged(double timeMs)
{
#if TOOLS_SIDEBAR_SHOWS_TIME
    this->lastTotalTime = timeMs;
    this->totalTime->setText(Transport::getTimeString(this->lastTotalTime.get()), dontSendNotification);
#endif
}

void SequencerSidebarRight::onLoopModeChanged(bool hasLoop, float startBeat, float endBeat)
{
    this->annotationsButton->setChecked(hasLoop);
}

void SequencerSidebarRight::onPlay()
{
#if TOOLS_SIDEBAR_SHOWS_TIME
    this->timerStartSystemTime = Time::getMillisecondCounter();
    this->timerStartSeekTime = this->lastSeekTime;
    this->startTimer(100);
#endif

    this->transportControl->showPlayingMode(true);
}

void SequencerSidebarRight::onRecord()
{
    this->transportControl->showRecordingMode(true);
}

void SequencerSidebarRight::onRecordFailed(const Array<MidiDeviceInfo> &devices)
{
    jassert(devices.size() != 1);

    if (!this->isShowing()) // shouldn't happen, but just in case it does
    {
        jassertfalse;
        return;
    }

    if (devices.isEmpty())
    {
        App::Layout().showTooltip(TRANS(I18n::Settings::midiNoInputDevices));
        this->transportControl->showRecordingError();
    }
    else
    {
        this->transportControl->showRecordingMenu(devices);
    }
}

void SequencerSidebarRight::onStop()
{
#if TOOLS_SIDEBAR_SHOWS_TIME
    this->stopTimer();
#endif

    this->transportControl->showPlayingMode(false);
    this->transportControl->showRecordingMode(false);
}

//===----------------------------------------------------------------------===//
// ChangeListener
//===----------------------------------------------------------------------===//

void SequencerSidebarRight::changeListenerCallback(ChangeBroadcaster *source)
{
    this->updateModeButtons();
}

//===----------------------------------------------------------------------===//
// Timer
//===----------------------------------------------------------------------===//

void SequencerSidebarRight::timerCallback()
{
    this->triggerAsyncUpdate();
}

//===----------------------------------------------------------------------===//
// SequencerSidebarRight
//===----------------------------------------------------------------------===//

void SequencerSidebarRight::updateModeButtons()
{
    this->recreateMenu();
    this->listBox->updateContent();
}

void SequencerSidebarRight::emitAnnotationsCallout(Component *newAnnotationsMenu)
{
    HelioCallout::emit(newAnnotationsMenu, this->annotationsButton.get());
}

void SequencerSidebarRight::setLinearMode()
{
    if (this->menuMode != MenuMode::PianoRollTools)
    {
        this->menuMode = MenuMode::PianoRollTools;
        this->recreateMenu();
        this->listBox->updateContent();
    }
}

void SequencerSidebarRight::setPatternMode()
{
    if (this->menuMode != MenuMode::PatternRollTools)
    {
        this->menuMode = MenuMode::PatternRollTools;
        this->recreateMenu();
        this->listBox->updateContent();
    }
}

//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SequencerSidebarRight" template="../../../Template"
                 componentName="" parentClasses="public Component, protected TransportListener, protected AsyncUpdater, protected ListBoxModel, protected ChangeListener, protected Timer"
                 constructorParams="ProjectNode &amp;parent" variableInitialisers="project(parent),&#10;lastSeekTime(0.0),&#10;lastTotalTime(0.0),&#10;timerStartSeekTime(0.0),&#10;timerStartSystemTime(0.0)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="44" initialHeight="640">
  <METHODS>
  </METHODS>
  <BACKGROUND backgroundColour="0"/>
  <GENERICCOMPONENT name="" id="381fa571a3dfc5cd" memberName="listBox" virtualName=""
                    explicitFocusOrder="0" pos="0 41 0M 121M" class="ListBox" params=""/>
  <JUCERCOMP name="" id="28ce45d9e84b729c" memberName="headLine" virtualName=""
             explicitFocusOrder="0" pos="0 39 0M 2" sourceFile="../../Themes/SeparatorHorizontalReversed.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="accf780c6ef7ae9e" memberName="shadow" virtualName=""
             explicitFocusOrder="0" pos="0 79Rr 0M 6" sourceFile="../../Themes/ShadowUpwards.cpp"
             constructorParams="Light"/>
  <JUCERCOMP name="" id="22d481533ce3ecd3" memberName="separator" virtualName=""
             explicitFocusOrder="0" pos="0 78Rr 0M 2" sourceFile="../../Themes/SeparatorHorizontal.cpp"
             constructorParams=""/>
  <LABEL name="" id="700073f74a17c931" memberName="totalTime" virtualName=""
         explicitFocusOrder="0" pos="80Cc 9Rr 72 18" labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="14.0" kerning="0.0" bold="0" italic="0" justification="36"/>
  <LABEL name="" id="b9e867ece7f52ad8" memberName="currentTime" virtualName=""
         explicitFocusOrder="0" pos="80Cc 26Rr 72 22" labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="16.0" kerning="0.0" bold="0" italic="0" justification="36"/>
  <JUCERCOMP name="" id="1d398dc12e2047bd" memberName="headShadow" virtualName=""
             explicitFocusOrder="0" pos="0 40 0M 6" sourceFile="../../Themes/ShadowDownwards.cpp"
             constructorParams="Light"/>
  <GENERICCOMPONENT name="" id="34c972d7b22acf17" memberName="annotationsButton"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 0 0M 39" class="MenuItemComponent"
                    params="this, nullptr, MenuItem::item(Icons::reprise, CommandIDs::ToggleLoopOverSelection)"/>
  <JUCERCOMP name="" id="bb2e14336f795a57" memberName="transportControl" virtualName=""
             explicitFocusOrder="0" pos="0 0Rr 0M 79" sourceFile="../../Common/TransportControlComponent.cpp"
             constructorParams="nullptr"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif



