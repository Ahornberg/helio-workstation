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
#include "AnnotationsSequence.h"
#include "AnnotationsProjectMap.h"
//[/Headers]

#include "AnnotationLargeComponent.h"

//[MiscUserDefs]
#define ANNOTATION_RESIZE_CORNER 10
//[/MiscUserDefs]

AnnotationLargeComponent::AnnotationLargeComponent(AnnotationsProjectMap &parent, const AnnotationEvent &targetEvent)
    : AnnotationComponent(parent, targetEvent)
{

    //[UserPreSize]
    this->setPaintingIsUnclipped(true);
    this->setInterceptsMouseClicks(true, false);
    this->setMouseClickGrabsKeyboardFocus(false);

    this->font = Font(16.f, Font::plain);
    //[/UserPreSize]

    this->setSize(128, 32);

    //[Constructor]
    //[/Constructor]
}

AnnotationLargeComponent::~AnnotationLargeComponent()
{
    //[Destructor_pre]
    //[/Destructor_pre]


    //[Destructor]
    //[/Destructor]
}

void AnnotationLargeComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
#if 0
    //[/UserPrePaint]

    {
        int x = 2, y = 1, width = getWidth() - 6, height = getHeight() - 8;
        String text (String());
        Colour fillColour = Colour (0x88ffffff);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (Font (16.00f, Font::plain));
        g.drawText (text, x, y, width, height,
                    Justification::centredLeft, true);
    }

    {
        int x = 0, y = 0, width = getWidth() - 0, height = 3;
        Colour fillColour = Colour (0x20ffffff);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRect (x, y, width, height);
    }

    //[UserPaint] Add your own custom painting code here..
#endif

    const Colour baseColour(findDefaultColour(Label::textColourId));

    g.setColour(this->event.getTrackColour()
        .interpolatedWith(baseColour, 0.5f).withAlpha(0.65f));

    g.fillRect(1.f, 0.f, float(this->getWidth() - 1), 3.f);

    if (this->event.getDescription().isNotEmpty())
    {
        const Font labelFont(16.f, Font::plain);
        g.setColour(this->event.getTrackColour().interpolatedWith(baseColour, 0.55f).withAlpha(0.9f));

        GlyphArrangement arr;
        arr.addFittedText(labelFont,
                          this->event.getDescription(),
                          2.f + this->boundsOffset.getX(),
                          0.f,
                          float(this->getWidth()) - 16.f,
                          float(this->getHeight()) - 8.f,
                          Justification::centredLeft,
                          1,
                          0.85f);
        arr.draw(g);
    }
    //[/UserPaint]
}

void AnnotationLargeComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AnnotationLargeComponent::mouseMove (const MouseEvent& e)
{
    //[UserCode_mouseMove] -- Add your code here...
    if (this->canResize() &&
        e.x >= (this->getWidth() - ANNOTATION_RESIZE_CORNER))
    {
        this->setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else
    {
        this->setMouseCursor(MouseCursor::PointingHandCursor);
    }
    //[/UserCode_mouseMove]
}

void AnnotationLargeComponent::mouseDown (const MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...
    this->mouseDownWasTriggered = true;

    if (e.mods.isLeftButtonDown())
    {
        if (this->canResize() &&
            e.x >= (this->getWidth() - ANNOTATION_RESIZE_CORNER))
        {
            this->state = State::ResizingRight;
            this->hadCheckpoint = false;
        }
        else
        {
            this->state = State::Dragging;
            this->dragger.startDraggingComponent(this, e);
            this->hadCheckpoint = false;
        }
    }
    else
    {
        this->editor.alternateActionFor(this);
    }
    //[/UserCode_mouseDown]
}

void AnnotationLargeComponent::mouseDrag (const MouseEvent& e)
{
    //[UserCode_mouseDrag] -- Add your code here...
    if (e.mods.isLeftButtonDown() && e.getDistanceFromDragStart() > 3)
    {
        if (this->state == State::ResizingRight)
        {
            const float newLength = jmax(0.f,
                this->editor.getBeatByXPosition(this->getX() + e.x) - this->event.getBeat());

            const bool lengthHasChanged = (this->event.getLength() != newLength);

            if (lengthHasChanged)
            {
                auto *sequence = static_cast<AnnotationsSequence *>(this->event.getSequence());

                if (!this->hadCheckpoint)
                {
                    sequence->checkpoint();
                    this->hadCheckpoint = true;
                }

                //DBG(newLength);
                sequence->change(this->event, this->event.withLength(newLength), true);
            }
        }
        else if (this->state == State::Dragging)
        {
            this->setMouseCursor(MouseCursor::DraggingHandCursor);
            this->dragger.dragComponent(this, e, nullptr);
            const float newBeat = this->editor.getBeatByXPosition(this->getX());
            const bool beatHasChanged = (this->event.getBeat() != newBeat);

            if (beatHasChanged)
            {
                const bool firstChangeIsToCome = !this->hadCheckpoint;
                auto *sequence = static_cast<AnnotationsSequence *>(this->event.getSequence());

                if (! this->hadCheckpoint)
                {
                    sequence->checkpoint();
                    this->hadCheckpoint = true;
                }

                // Drag-and-copy logic:
                if (firstChangeIsToCome && e.mods.isAnyModifierKeyDown())
                {
                    sequence->insert(this->event.copyWithNewId(), true);
                }

                sequence->change(this->event, this->event.withBeat(newBeat), true);
            }
            else
            {
                this->editor.alignAnnotationComponent(this);
            }
        }
    }
    //[/UserCode_mouseDrag]
}

void AnnotationLargeComponent::mouseUp (const MouseEvent& e)
{
    //[UserCode_mouseUp] -- Add your code here...
    if (e.mods.isLeftButtonDown())
    {
        if (this->state == State::Dragging ||
            this->state == State::ResizingRight)
        {
            this->state = State::None;
            this->setMouseCursor(MouseCursor::PointingHandCursor);
            this->editor.onAnnotationMoved(this);
        }

        if (e.getDistanceFromDragStart() < 10 &&
            this->mouseDownWasTriggered &&
            !this->hadCheckpoint)
        {
            this->editor.onAnnotationTapped(this);
        }
    }

    this->mouseDownWasTriggered = false;
    //[/UserCode_mouseUp]
}

void AnnotationLargeComponent::mouseDoubleClick (const MouseEvent& e)
{
    //[UserCode_mouseDoubleClick] -- Add your code here...
    //[/UserCode_mouseDoubleClick]
}


//[MiscUserCode]

void AnnotationLargeComponent::setRealBounds(const Rectangle<float> bounds)
{
    Rectangle<int> intBounds(bounds.toType<int>());
    this->boundsOffset = {
        bounds.getX() - float(intBounds.getX()),
        bounds.getY(),
        bounds.getWidth() - float(intBounds.getWidth()),
        bounds.getHeight()
    };

    this->setBounds(intBounds);
}

void AnnotationLargeComponent::updateContent()
{
    if (this->text != this->event.getDescription())
    {
        this->text = this->event.getDescription();
        this->textWidth = float(this->font.getStringWidth(this->event.getDescription()));
    }

    this->repaint();
}

float AnnotationLargeComponent::getTextWidth() const noexcept
{
    return this->textWidth;
}

bool AnnotationLargeComponent::canResize() const noexcept
{
    return (this->getWidth() >= (ANNOTATION_RESIZE_CORNER * 2));
}

//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AnnotationLargeComponent"
                 template="../../../../Template" componentName="" parentClasses="public AnnotationComponent"
                 constructorParams="AnnotationsProjectMap &amp;parent, const AnnotationEvent &amp;targetEvent"
                 variableInitialisers="AnnotationComponent(parent, targetEvent)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="128" initialHeight="32">
  <METHODS>
    <METHOD name="mouseDown (const MouseEvent&amp; e)"/>
    <METHOD name="mouseDrag (const MouseEvent&amp; e)"/>
    <METHOD name="mouseUp (const MouseEvent&amp; e)"/>
    <METHOD name="mouseMove (const MouseEvent&amp; e)"/>
    <METHOD name="mouseDoubleClick (const MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="0">
    <TEXT pos="2 1 6M 8M" fill="solid: 88ffffff" hasStroke="0" text=""
          fontname="Default font" fontsize="16.0" kerning="0.0" bold="0"
          italic="0" justification="33"/>
    <RECT pos="0 0 0M 3" fill="solid: 20ffffff" hasStroke="0"/>
  </BACKGROUND>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif



