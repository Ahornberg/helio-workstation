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

#include "ProjectPage.h"

//[MiscUserDefs]
#include "VersionControlNode.h"
#include "PlayerThread.h"
#include "ProjectNode.h"
#include "ProjectMetadata.h"
#include "HelioTheme.h"
#include "HelioCallout.h"
#include "ProjectMenu.h"
#include "CommandIDs.h"

static String getTimeString(const RelativeTime &time)
{
    String res;

    int n = std::abs(int(time.inMinutes()));

    if (n > 0)
    {
        res = res + TRANS_PLURAL("{x} minutes", n) + " ";
    }

    n = std::abs(int(time.inSeconds())) % 60;
    res = res + TRANS_PLURAL("{x} seconds", n) + " ";

    return res;
}
//[/MiscUserDefs]

ProjectPage::ProjectPage(ProjectNode &parentProject)
    : project(parentProject)
{
    this->background.reset(new PanelBackgroundB());
    this->addAndMakeVisible(background.get());
    this->projectTitleEditor.reset(new Label(String(),
                                              String()));
    this->addAndMakeVisible(projectTitleEditor.get());
    this->projectTitleEditor->setFont(Font (37.00f, Font::plain));
    projectTitleEditor->setJustificationType(Justification::topLeft);
    projectTitleEditor->setEditable(true, true, false);
    this->projectTitleEditor->addListener(this);

    this->projectTitleLabel.reset(new Label(String(),
                                             String()));
    this->addAndMakeVisible(projectTitleLabel.get());
    this->projectTitleLabel->setFont(Font (21.00f, Font::plain));
    projectTitleLabel->setJustificationType(Justification::topRight);
    projectTitleLabel->setEditable(false, false, false);

    this->authorEditor.reset(new Label(String(),
                                        String()));
    this->addAndMakeVisible(authorEditor.get());
    this->authorEditor->setFont(Font (37.00f, Font::plain));
    authorEditor->setJustificationType(Justification::topLeft);
    authorEditor->setEditable(true, true, false);
    this->authorEditor->addListener(this);

    this->authorLabel.reset(new Label(String(),
                                       String()));
    this->addAndMakeVisible(authorLabel.get());
    this->authorLabel->setFont(Font (21.00f, Font::plain));
    authorLabel->setJustificationType(Justification::topRight);
    authorLabel->setEditable(false, false, false);

    this->descriptionEditor.reset(new Label(String(),
                                             String()));
    this->addAndMakeVisible(descriptionEditor.get());
    this->descriptionEditor->setFont(Font (37.00f, Font::plain));
    descriptionEditor->setJustificationType(Justification::topLeft);
    descriptionEditor->setEditable(true, true, false);
    this->descriptionEditor->addListener(this);

    this->descriptionLabel.reset(new Label(String(),
                                            String()));
    this->addAndMakeVisible(descriptionLabel.get());
    this->descriptionLabel->setFont(Font (21.00f, Font::plain));
    descriptionLabel->setJustificationType(Justification::topRight);
    descriptionLabel->setEditable(false, false, false);

    this->locationLabel.reset(new Label(String(),
                                         String()));
    this->addAndMakeVisible(locationLabel.get());
    this->locationLabel->setFont(Font (16.00f, Font::plain));
    locationLabel->setJustificationType(Justification::topRight);
    locationLabel->setEditable(false, false, true);

    this->locationText.reset(new Label(String(),
                                        String()));
    this->addAndMakeVisible(locationText.get());
    this->locationText->setFont(Font (16.00f, Font::plain));
    locationText->setJustificationType(Justification::topLeft);
    locationText->setEditable(false, false, true);

    this->contentStatsLabel.reset(new Label(String(),
                                             String()));
    this->addAndMakeVisible(contentStatsLabel.get());
    this->contentStatsLabel->setFont(Font (16.00f, Font::plain));
    contentStatsLabel->setJustificationType(Justification::topRight);
    contentStatsLabel->setEditable(false, false, true);

    this->contentStatsText.reset(new Label(String(),
                                            String()));
    this->addAndMakeVisible(contentStatsText.get());
    this->contentStatsText->setFont(Font (16.00f, Font::plain));
    contentStatsText->setJustificationType(Justification::topLeft);
    contentStatsText->setEditable(false, false, true);

    this->vcsStatsLabel.reset(new Label(String(),
                                         String()));
    this->addAndMakeVisible(vcsStatsLabel.get());
    this->vcsStatsLabel->setFont(Font (16.00f, Font::plain));
    vcsStatsLabel->setJustificationType(Justification::topRight);
    vcsStatsLabel->setEditable(false, false, true);

    this->vcsStatsText.reset(new Label(String(),
                                        String()));
    this->addAndMakeVisible(vcsStatsText.get());
    this->vcsStatsText->setFont(Font (16.00f, Font::plain));
    vcsStatsText->setJustificationType(Justification::topLeft);
    vcsStatsText->setEditable(false, false, true);

    this->startTimeLabel.reset(new Label(String(),
                                          String()));
    this->addAndMakeVisible(startTimeLabel.get());
    this->startTimeLabel->setFont(Font (16.00f, Font::plain));
    startTimeLabel->setJustificationType(Justification::topRight);
    startTimeLabel->setEditable(false, false, true);

    this->startTimeText.reset(new Label(String(),
                                         String()));
    this->addAndMakeVisible(startTimeText.get());
    this->startTimeText->setFont(Font (16.00f, Font::plain));
    startTimeText->setJustificationType(Justification::topLeft);
    startTimeText->setEditable(false, false, true);

    this->lengthLabel.reset(new Label(String(),
                                       String()));
    this->addAndMakeVisible(lengthLabel.get());
    this->lengthLabel->setFont(Font (16.00f, Font::plain));
    lengthLabel->setJustificationType(Justification::topRight);
    lengthLabel->setEditable(false, false, true);

    this->lengthText.reset(new Label(String(),
                                      String()));
    this->addAndMakeVisible(lengthText.get());
    this->lengthText->setFont(Font (16.00f, Font::plain));
    lengthText->setJustificationType(Justification::topLeft);
    lengthText->setEditable(false, false, true);

    this->level1.reset(new Component());
    this->addAndMakeVisible(level1.get());
    level1->setName ("level1");

    this->level2.reset(new Component());
    this->addAndMakeVisible(level2.get());
    level2->setName ("level2");

    this->licenseLabel.reset(new Label(String(),
                                        String()));
    this->addAndMakeVisible(licenseLabel.get());
    this->licenseLabel->setFont(Font (21.00f, Font::plain));
    licenseLabel->setJustificationType(Justification::topRight);
    licenseLabel->setEditable(false, false, true);

    this->licenseEditor.reset(new Label(String(),
                                         String()));
    this->addAndMakeVisible(licenseEditor.get());
    this->licenseEditor->setFont(Font (37.00f, Font::plain));
    licenseEditor->setJustificationType(Justification::topLeft);
    licenseEditor->setEditable(true, true, false);
    this->licenseEditor->addListener(this);

    this->menuButton.reset(new MenuButton());
    this->addAndMakeVisible(menuButton.get());
    this->revealLocationButton.reset(new ImageButton(String()));
    this->addAndMakeVisible(revealLocationButton.get());
    revealLocationButton->addListener(this);

    revealLocationButton->setImages (false, true, false,
                                     Image(), 1.000f, Colour (0x00000000),
                                     Image(), 1.000f, Colour (0x00000000),
                                     Image(), 1.000f, Colour (0x00000000));

    //[UserPreSize]
    this->licenseLabel->setText(TRANS(I18n::Page::projectLicense), dontSendNotification);
    this->lengthLabel->setText(TRANS(I18n::Page::projectDuration), dontSendNotification);
    this->startTimeLabel->setText(TRANS(I18n::Page::projectStartdate), dontSendNotification);
    this->vcsStatsLabel->setText(TRANS(I18n::Page::projectStatsVcs), dontSendNotification);
    this->contentStatsLabel->setText(TRANS(I18n::Page::projectStatsContent), dontSendNotification);
    this->locationLabel->setText(TRANS(I18n::Page::projectFilelocation), dontSendNotification);
    this->descriptionLabel->setText(TRANS(I18n::Page::projectDescription), dontSendNotification);
    this->authorLabel->setText(TRANS(I18n::Page::projectAuthor), dontSendNotification);
    this->projectTitleLabel->setText(TRANS(I18n::Page::projectTitle), dontSendNotification);

    this->project.addChangeListener(this);
    this->project.getTransport().addTransportListener(this);

    this->setWantsKeyboardFocus(true);
    this->setFocusContainer(true);

    this->revealLocationButton->setMouseCursor(MouseCursor::PointingHandCursor);

    // TODO remove this button?
    this->menuButton->setVisible(false);
    //[/UserPreSize]

    this->setSize(600, 400);

    //[Constructor]
#if HELIO_MOBILE
    // не комильфо на мобильниках показывать расположение файлв
    this->locationLabel->setVisible(false);
    this->locationText->setVisible(false);
#endif
    //[/Constructor]
}

ProjectPage::~ProjectPage()
{
    //[Destructor_pre]
    this->project.getTransport().removeTransportListener(this);
    this->project.removeChangeListener(this);
    //[/Destructor_pre]

    background = nullptr;
    projectTitleEditor = nullptr;
    projectTitleLabel = nullptr;
    authorEditor = nullptr;
    authorLabel = nullptr;
    descriptionEditor = nullptr;
    descriptionLabel = nullptr;
    locationLabel = nullptr;
    locationText = nullptr;
    contentStatsLabel = nullptr;
    contentStatsText = nullptr;
    vcsStatsLabel = nullptr;
    vcsStatsText = nullptr;
    startTimeLabel = nullptr;
    startTimeText = nullptr;
    lengthLabel = nullptr;
    lengthText = nullptr;
    level1 = nullptr;
    level2 = nullptr;
    licenseLabel = nullptr;
    licenseEditor = nullptr;
    menuButton = nullptr;
    revealLocationButton = nullptr;

    //[Destructor]
    //[/Destructor]
}

void ProjectPage::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ProjectPage::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    background->setBounds(0, 0, getWidth() - 0, getHeight() - 0);
    projectTitleEditor->setBounds((getWidth() / 2) + -100, proportionOfHeight (0.0758f) + 20, 440, 48);
    projectTitleLabel->setBounds((getWidth() / 2) + -104 - 202, proportionOfHeight (0.0758f) + 0, 202, 48);
    authorEditor->setBounds((getWidth() / 2) + -100, proportionOfHeight (0.0758f) + 90, 440, 48);
    authorLabel->setBounds((getWidth() / 2) + -104 - 202, proportionOfHeight (0.0758f) + 70, 202, 48);
    descriptionEditor->setBounds((getWidth() / 2) + -100, proportionOfHeight (0.0758f) + 160, 440, 48);
    descriptionLabel->setBounds((getWidth() / 2) + -104 - 202, proportionOfHeight (0.0758f) + 140, 202, 48);
    locationLabel->setBounds((getWidth() / 2) + -104 - 300, (proportionOfHeight (0.0758f) + 310) + 138, 300, 32);
    locationText->setBounds((getWidth() / 2) + -100, (proportionOfHeight (0.0758f) + 310) + 138, 400, 96);
    contentStatsLabel->setBounds((getWidth() / 2) + -104 - 300, (proportionOfHeight (0.0758f) + 310) + 106, 300, 32);
    contentStatsText->setBounds((getWidth() / 2) + -100, (proportionOfHeight (0.0758f) + 310) + 106, 400, 32);
    vcsStatsLabel->setBounds((getWidth() / 2) + -104 - 300, (proportionOfHeight (0.0758f) + 310) + 76, 300, 32);
    vcsStatsText->setBounds((getWidth() / 2) + -100, (proportionOfHeight (0.0758f) + 310) + 76, 400, 32);
    startTimeLabel->setBounds((getWidth() / 2) + -104 - 300, (proportionOfHeight (0.0758f) + 310) + 46, 300, 32);
    startTimeText->setBounds((getWidth() / 2) + -100, (proportionOfHeight (0.0758f) + 310) + 46, 400, 32);
    lengthLabel->setBounds((getWidth() / 2) + -104 - 300, (proportionOfHeight (0.0758f) + 310) + 16, 300, 32);
    lengthText->setBounds((getWidth() / 2) + -100, (proportionOfHeight (0.0758f) + 310) + 16, 400, 32);
    level1->setBounds(32, proportionOfHeight (0.0758f), 150, 24);
    level2->setBounds(32, proportionOfHeight (0.0758f) + 310, 150, 24);
    licenseLabel->setBounds((getWidth() / 2) + -104 - 202, proportionOfHeight (0.0758f) + 210, 202, 48);
    licenseEditor->setBounds((getWidth() / 2) + -100, proportionOfHeight (0.0758f) + 230, 440, 48);
    menuButton->setBounds((getWidth() / 2) - (128 / 2), getHeight() - -16 - (128 / 2), 128, 128);
    revealLocationButton->setBounds(((getWidth() / 2) + -100) + -150, ((proportionOfHeight (0.0758f) + 310) + 138) + 0, 400 - -150, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ProjectPage::labelTextChanged(Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == projectTitleEditor.get())
    {
        //[UserLabelCode_projectTitleEditor] -- add your label text handling code here..
        if (labelThatHasChanged->getText().isNotEmpty())
        {
            this->project.getProjectInfo()->setFullName(labelThatHasChanged->getText());
        }
        else
        {
            const String &fullname = this->project.getProjectInfo()->getFullName();
            labelThatHasChanged->setText(fullname, sendNotification);
        }
        //[/UserLabelCode_projectTitleEditor]
    }
    else if (labelThatHasChanged == authorEditor.get())
    {
        //[UserLabelCode_authorEditor] -- add your label text handling code here..
        this->project.getProjectInfo()->setAuthor(labelThatHasChanged->getText());
        //[/UserLabelCode_authorEditor]
    }
    else if (labelThatHasChanged == descriptionEditor.get())
    {
        //[UserLabelCode_descriptionEditor] -- add your label text handling code here..
        this->project.getProjectInfo()->setDescription(labelThatHasChanged->getText());
        //[/UserLabelCode_descriptionEditor]
    }
    else if (labelThatHasChanged == licenseEditor.get())
    {
        //[UserLabelCode_licenseEditor] -- add your label text handling code here..
        this->project.getProjectInfo()->setLicense(labelThatHasChanged->getText());
        //[/UserLabelCode_licenseEditor]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void ProjectPage::buttonClicked(Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == revealLocationButton.get())
    {
        //[UserButtonCode_revealLocationButton] -- add your button handler code here..
        this->project.getDocument()->getFile().revealToUser();
        //[/UserButtonCode_revealLocationButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void ProjectPage::visibilityChanged()
{
    //[UserCode_visibilityChanged] -- Add your code here...
    const RelativeTime totalTime(this->totalTimeMs.get() / 1000.0);
    this->lengthText->setText(getTimeString(totalTime), dontSendNotification);
    //[/UserCode_visibilityChanged]
}

void ProjectPage::handleCommandMessage (int commandId)
{
    //[UserCode_handleCommandMessage] -- Add your code here...
    if (commandId == CommandIDs::MenuButtonPressed)
    {
        MenuPanel *panel = new ProjectMenu(this->project, MenuPanel::SlideUp);
        HelioCallout::emit(panel, this->menuButton.get());
    }
    //[/UserCode_handleCommandMessage]
}


//[MiscUserCode]
void ProjectPage::updateContent()
{
    const String &fullname = this->project.getProjectInfo()->getFullName();
    const String &author = this->project.getProjectInfo()->getAuthor();
    const String &description = this->project.getProjectInfo()->getDescription();
    const String &license = this->project.getProjectInfo()->getLicense();
    const String &startTime = App::getHumanReadableDate(Time(this->project.getProjectInfo()->getStartTimestamp()));

#if HELIO_DESKTOP
    const String &clickToEdit = TRANS(I18n::Page::projectDefaultValueDesktop);
#elif HELIO_MOBILE
    const String &clickToEdit = TRANS(I18n::Page::projectDefaultValueMobile);
#endif

    this->projectTitleEditor->setText(fullname.isEmpty() ? clickToEdit : fullname, dontSendNotification);
    this->authorEditor->setText(author.isEmpty() ? TRANS(I18n::Page::projectDefaultAuthor) : author, dontSendNotification);
    this->descriptionEditor->setText(description.isEmpty() ? clickToEdit : description, dontSendNotification);
    this->licenseEditor->setText(license.isEmpty() ? TRANS(I18n::Page::projectDefaultLicense) : license, dontSendNotification);

    this->startTimeText->setText(startTime, dontSendNotification);
    this->locationText->setText(this->project.getDocument()->getFullPath(), dontSendNotification);
    this->contentStatsText->setText(this->project.getStats(), dontSendNotification);

    if (auto *vcti = this->project.findChildOfType<VersionControlNode>())
    {
        this->vcsStatsText->setText(vcti->getStatsString(), dontSendNotification);
    }
}

void ProjectPage::onSeek(float beatPosition, double currentTimeMs, double totalTimeMs)
{
    this->totalTimeMs = totalTimeMs;
}

void ProjectPage::onTotalTimeChanged(double totalTimeMs) noexcept
{
    this->totalTimeMs = totalTimeMs;
}

void ProjectPage::changeListenerCallback(ChangeBroadcaster *source)
{
    this->updateContent();
}
//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ProjectPage" template="../../../Template"
                 componentName="" parentClasses="public Component, protected TransportListener, protected ChangeListener"
                 constructorParams="ProjectNode &amp;parentProject" variableInitialisers="project(parentProject)"
                 snapPixels="4" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <METHODS>
    <METHOD name="handleCommandMessage (int commandId)"/>
    <METHOD name="visibilityChanged()"/>
  </METHODS>
  <BACKGROUND backgroundColour="0"/>
  <JUCERCOMP name="" id="e130bb0b9ed67f09" memberName="background" virtualName=""
             explicitFocusOrder="0" pos="0 0 0M 0M" sourceFile="../../Themes/PanelBackgroundB.cpp"
             constructorParams=""/>
  <LABEL name="" id="a162c9dbc90775e7" memberName="projectTitleEditor"
         virtualName="" explicitFocusOrder="0" pos="-100C 20 440 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Default font" fontsize="37.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="b93b5ef0dc95ee24" memberName="projectTitleLabel"
         virtualName="" explicitFocusOrder="0" pos="-104Cr 0 202 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="21.0"
         kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="9c63b5388edfe183" memberName="authorEditor" virtualName=""
         explicitFocusOrder="0" pos="-100C 90 440 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Default font" fontsize="37.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="cf32360d33639f7f" memberName="authorLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 70 202 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="21.0"
         kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="5b9fd0ca53fe4337" memberName="descriptionEditor"
         virtualName="" explicitFocusOrder="0" pos="-100C 160 440 48"
         posRelativeY="b6ea6ccc6b9be1f8" labelText="" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Default font"
         fontsize="37.0" kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="1a7ebced267e73e8" memberName="descriptionLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 140 202 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="21.0" kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="cf836ffeded76ad1" memberName="locationLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 138 300 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="1" fontname="Default font"
         fontsize="16.0" kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="e68c5a019e000a0b" memberName="locationText" virtualName=""
         explicitFocusOrder="0" pos="-100C 138 400 96" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="16.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="e824154c21ea01f1" memberName="contentStatsLabel"
         virtualName="" explicitFocusOrder="0" pos="-104Cr 106 300 32"
         posRelativeY="91994c13c1a34ef8" labelText=""
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="1"
         fontname="Default font" fontsize="16.0" kerning="0.0" bold="0"
         italic="0" justification="10"/>
  <LABEL name="" id="4c13747d72949ab9" memberName="contentStatsText" virtualName=""
         explicitFocusOrder="0" pos="-100C 106 400 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="16.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="fa70bc89acdb3acf" memberName="vcsStatsLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 76 300 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="1" fontname="Default font"
         fontsize="16.0" kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="6e9d6e323ae75809" memberName="vcsStatsText" virtualName=""
         explicitFocusOrder="0" pos="-100C 76 400 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="16.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="1ca8b26361947f73" memberName="startTimeLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 46 300 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="1" fontname="Default font"
         fontsize="16.0" kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="5c9cf4e334ddde90" memberName="startTimeText" virtualName=""
         explicitFocusOrder="0" pos="-100C 46 400 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="16.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <LABEL name="" id="54f9aec3fdb83582" memberName="lengthLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 16 300 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="16.0"
         kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="3849b372a1b522da" memberName="lengthText" virtualName=""
         explicitFocusOrder="0" pos="-100C 16 400 32" posRelativeY="91994c13c1a34ef8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="16.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <GENERICCOMPONENT name="level1" id="b6ea6ccc6b9be1f8" memberName="level1" virtualName=""
                    explicitFocusOrder="0" pos="32 7.583% 150 24" class="Component"
                    params=""/>
  <GENERICCOMPONENT name="level2" id="91994c13c1a34ef8" memberName="level2" virtualName=""
                    explicitFocusOrder="0" pos="32 310 150 24" posRelativeY="b6ea6ccc6b9be1f8"
                    class="Component" params=""/>
  <LABEL name="" id="ed8bce664dddb1d1" memberName="licenseLabel" virtualName=""
         explicitFocusOrder="0" pos="-104Cr 210 202 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="1" fontname="Default font" fontsize="21.0"
         kerning="0.0" bold="0" italic="0" justification="10"/>
  <LABEL name="" id="63b4a599dbfa30da" memberName="licenseEditor" virtualName=""
         explicitFocusOrder="0" pos="-100C 230 440 48" posRelativeY="b6ea6ccc6b9be1f8"
         labelText="" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Default font" fontsize="37.0"
         kerning="0.0" bold="0" italic="0" justification="9"/>
  <JUCERCOMP name="" id="2ce00deefdf277e6" memberName="menuButton" virtualName=""
             explicitFocusOrder="0" pos="0Cc -16Rc 128 128" sourceFile="../../Common/MenuButton.cpp"
             constructorParams=""/>
  <IMAGEBUTTON name="" id="6ab32d6eda6c96a8" memberName="revealLocationButton"
               virtualName="" explicitFocusOrder="0" pos="-150 0 -150M 24" posRelativeX="e68c5a019e000a0b"
               posRelativeY="cf836ffeded76ad1" posRelativeW="e68c5a019e000a0b"
               buttonText="" connectedEdges="0" needsCallback="1" radioGroupId="0"
               keepProportions="0" resourceNormal="" opacityNormal="1.0" colourNormal="0"
               resourceOver="" opacityOver="1.0" colourOver="0" resourceDown=""
               opacityDown="1.0" colourDown="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
