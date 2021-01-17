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

#include "TranslationSettingsItem.h"

//[MiscUserDefs]
#include "Config.h"
#include "ColourIDs.h"
#include "IconComponent.h"

class TranslationSettingsItemSelection final : public Component
{
public:

    TranslationSettingsItemSelection()
    {
        this->setPaintingIsUnclipped(true);

        this->iconComponent = make<IconComponent>(Icons::apply);
        this->addAndMakeVisible(this->iconComponent.get());
        this->iconComponent->setIconAlphaMultiplier(0.6f);
    }

    void paint(Graphics &g) override
    {
        g.setColour(findDefaultColour(ColourIDs::Common::borderLineLight));
        g.fillRoundedRectangle(40.f, 2.f, float(this->getWidth() - 45), float(this->getHeight() - 5), 2.0f);
    }

    void resized() override
    {
        constexpr auto size = 28;
        this->iconComponent->setBounds(6, (this->getHeight() / 2) - (size / 2) - 1, size, size);
    }

private:

    UniquePointer<IconComponent> iconComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TranslationSettingsItemSelection)
};

class TranslationSettingsItemHighlighter final : public Component
{
public:

    TranslationSettingsItemHighlighter()
    {
        this->setPaintingIsUnclipped(true);
    }

    void paint(Graphics &g) override
    {
        g.setColour(findDefaultColour(ColourIDs::Common::borderLineLight));
        g.fillRoundedRectangle(40.f, 2.f, float(this->getWidth() - 45), float(this->getHeight() - 5), 2.f);
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TranslationSettingsItemHighlighter)
};

//[/MiscUserDefs]

TranslationSettingsItem::TranslationSettingsItem(ListBox &parentListBox)
    : DraggingListBoxComponent(parentListBox.getViewport())
{
    this->localeLabel.reset(new Label(String(),
                                       String()));
    this->addAndMakeVisible(localeLabel.get());
    this->localeLabel->setFont(Font (21.00f, Font::plain));
    localeLabel->setJustificationType(Justification::centredLeft);
    localeLabel->setEditable(false, false, false);

    this->idLabel.reset(new Label(String(),
                                   String()));
    this->addAndMakeVisible(idLabel.get());
    this->idLabel->setFont(Font (21.00f, Font::plain));
    idLabel->setJustificationType(Justification::centredRight);
    idLabel->setEditable(false, false, false);

    this->separator.reset(new SeparatorHorizontal());
    this->addAndMakeVisible(separator.get());

    //[UserPreSize]
    this->selectionComponent = make<TranslationSettingsItemSelection>();
    this->addChildComponent(this->selectionComponent.get());
    //[/UserPreSize]

    this->setSize(350, 32);

    //[Constructor]
    //[/Constructor]
}

TranslationSettingsItem::~TranslationSettingsItem()
{
    //[Destructor_pre]
    //this->selectionComponent = nullptr;
    //[/Destructor_pre]

    localeLabel = nullptr;
    idLabel = nullptr;
    separator = nullptr;

    //[Destructor]
    //[/Destructor]
}

void TranslationSettingsItem::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TranslationSettingsItem::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    localeLabel->setBounds(48, 0, proportionOfWidth (0.3800f), getHeight() - 2);
    idLabel->setBounds(getWidth() - 12 - proportionOfWidth (0.4771f), 0, proportionOfWidth (0.4771f), getHeight() - 2);
    separator->setBounds(55, getHeight() - 2, getWidth() - 65, 2);
    //[UserResized] Add your own custom resize handling here..
    this->selectionComponent->setBounds(this->getLocalBounds());
    //[/UserResized]
}


//[MiscUserCode]

void TranslationSettingsItem::setSelected(bool shouldBeSelected)
{
    if (shouldBeSelected)
    {
        App::Config().getTranslations()->loadLocaleWithId(this->idLabel->getText());
        App::recreateLayout();
    }
}

void TranslationSettingsItem::updateDescription(bool isLastRowInList, bool isCurrentLocale,
    const String &localeName, const String &localeId)
{
    this->separator->setVisible(!isLastRowInList);
    this->localeLabel->setText(localeName, dontSendNotification);
    this->idLabel->setText(localeId, dontSendNotification);

    if (isCurrentLocale)
    {
        this->selectionAnimator.fadeIn(this->selectionComponent.get(), Globals::UI::fadeInShort);
    }
    else
    {
        this->selectionAnimator.fadeOut(this->selectionComponent.get(), Globals::UI::fadeOutShort);
    }
}

Component *TranslationSettingsItem::createHighlighterComponent()
{
    return new TranslationSettingsItemHighlighter();
}

//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TranslationSettingsItem"
                 template="../../../Template" componentName="" parentClasses="public DraggingListBoxComponent"
                 constructorParams="ListBox &amp;parentListBox" variableInitialisers="DraggingListBoxComponent(parentListBox.getViewport())"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="350" initialHeight="32">
  <BACKGROUND backgroundColour="0"/>
  <LABEL name="" id="c261305e2de1ebf2" memberName="localeLabel" virtualName=""
         explicitFocusOrder="0" pos="48 0 38% 2M" labelText="" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="21.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <LABEL name="" id="a7e8c6a3ddd9ea22" memberName="idLabel" virtualName=""
         explicitFocusOrder="0" pos="12Rr 0 47.714% 2M" labelText=""
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="21.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="34"/>
  <JUCERCOMP name="" id="6f5a73e394d91c2a" memberName="separator" virtualName=""
             explicitFocusOrder="0" pos="55 0Rr 65M 2" sourceFile="../../Themes/SeparatorHorizontal.cpp"
             constructorParams=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
