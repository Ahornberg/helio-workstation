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
#include "HelioCallout.h"
#include "MainLayout.h"
#include "ColourIDs.h"

static int kClickCounterOnPopupClose = 0;
static int kClickCounterOnPopupStart = 0;

HelioCallout::HelioCallout(Component &c, Component *pointAtComponent,
    MainLayout *parentWorkspace, bool shouldAlignToMouse) :
    contentComponent(c),
    targetComponent(pointAtComponent),
    alignsToMouse(shouldAlignToMouse)
{
    jassert(parentWorkspace);

    this->setFocusContainerType(Component::FocusContainerType::none);
    this->setWantsKeyboardFocus(false);
    this->setInterceptsMouseClicks(false, true);
    this->setPaintingIsUnclipped(true);

    kClickCounterOnPopupStart = Desktop::getInstance().getMouseButtonClickCounter();
        
    const auto area = this->targetComponent->getScreenBounds();
    this->addAndMakeVisible(this->contentComponent);
    
    if (parentWorkspace != nullptr)
    {
        const auto b = parentWorkspace->getScreenBounds();

#if PLATFORM_DESKTOP
        const auto p = Desktop::getInstance().getMainMouseSource().getScreenPosition() - b.getPosition().toFloat();
#elif PLATFORM_MOBILE
        const auto p = Desktop::getInstance().getMainMouseSource().getLastMouseDownPosition() - b.getPosition().toFloat();
#endif

        this->clickPointAbs = Point<float>(p.getX() / float(b.getWidth()), p.getY() / float(b.getHeight()));

        parentWorkspace->addChildComponent(this);
        this->findTargetPointAndUpdateBounds();
        this->setVisible(true);
    }
    else
    {
        const auto &displays = Desktop::getInstance().getDisplays();
        this->setAlwaysOnTop(true);
        this->pointToAndFit(area, displays.getDisplayForPoint(area.getCentre())->userArea);
        this->addToDesktop(ComponentPeer::windowIsTemporary);
    }
}

HelioCallout::~HelioCallout()
{
    kClickCounterOnPopupClose = Desktop::getInstance().getMouseButtonClickCounter();
}

class HelioCallOutCallback final : public ModalComponentManager::Callback
{
public:
    HelioCallOutCallback(Component *c, Component *pointAtComponent,
                         MainLayout *parentWorkspace, bool shouldAlignToMouse) :
        content(c),
        callout(*c, pointAtComponent, parentWorkspace, shouldAlignToMouse)
    {
        this->callout.setVisible(true);
        this->callout.enterModalState(true, this);
    }
    
    void modalStateFinished(int) override {}

    UniquePointer<Component> content;
    HelioCallout callout;
    
    JUCE_DECLARE_NON_COPYABLE(HelioCallOutCallback)
};


void HelioCallout::setArrowSize(const float newSize)
{
    this->arrowSize = newSize;
    this->updateShape();
}

int HelioCallout::getBorderSize() const noexcept
{
    return int(this->arrowSize + 1);
    //return jmax(20, int(this->arrowSize));
}

void HelioCallout::fadeIn()
{
    Desktop::getInstance().getAnimator().animateComponent(this,
        this->getBounds(), 1.f, Globals::UI::fadeInLong, false, 0.0, 0.0);
}

void HelioCallout::fadeOut()
{
    const int reduceBy = 20;
    const auto offset = this->targetPoint - this->getBounds().getCentre().toFloat();
    const auto offsetNormalized = (offset / offset.getDistanceFromOrigin() * reduceBy).toInt();
    
    Desktop::getInstance().getAnimator().animateComponent(this,
        this->getBounds().reduced(reduceBy).translated(offsetNormalized.getX(), offsetNormalized.getY()),
        0.f, Globals::UI::fadeOutLong, true, 0.0, 0.0);
}


//===----------------------------------------------------------------------===//
// Static
//===----------------------------------------------------------------------===//

#define CALLOUT_FRAME_MARGIN (2)

void HelioCallout::emit(Component *newComponent,
                        Component *pointAtComponent,
                        bool alignsToMousePosition)
{
    jassert(newComponent != nullptr);
    
    HelioCallout &cb =
    (new HelioCallOutCallback(newComponent,
                              pointAtComponent,
                              &App::Layout(),
                              alignsToMousePosition))->callout;
    
    cb.setAlpha(0.f);
    cb.fadeIn();
}

int HelioCallout::numClicksSinceLastStartedPopup()
{
    return Desktop::getInstance().getMouseButtonClickCounter() - kClickCounterOnPopupStart;
}

int HelioCallout::numClicksSinceLastClosedPopup()
{
    return Desktop::getInstance().getMouseButtonClickCounter() - kClickCounterOnPopupClose;
}


//===----------------------------------------------------------------------===//
// Component
//===----------------------------------------------------------------------===//

void HelioCallout::paint(Graphics& g)
{
    g.setColour(findDefaultColour(ColourIDs::Callout::fill));
    g.fillPath(this->outline);

    g.setColour(findDefaultColour(ColourIDs::Callout::frame));
    g.strokePath(this->outline, PathStrokeType(1.f));
}

void HelioCallout::resized()
{
    const int borderSpace = this->getBorderSize();
    this->contentComponent.setTopLeftPosition(borderSpace, borderSpace);
    this->updateShape();
}

void HelioCallout::moved()
{
    this->updateShape();
}

void HelioCallout::parentSizeChanged()
{
    this->findTargetPointAndUpdateBounds();
}

void HelioCallout::childBoundsChanged(Component *)
{
    this->pointToAndFit(this->lastGoodTargetArea, this->lastGoodAvailableArea);
}

bool HelioCallout::hitTest(int x, int y)
{
    return this->outline.contains(float(x), float(y));
}

void HelioCallout::inputAttemptWhenModal()
{
    //const Point<int> mousePos(getMouseXYRelative() + getBounds().getPosition());
    //const bool shouldBeDismissedAsyncronously = this->targetArea.contains(mousePos) || this->alignsToMouse;
    const bool shouldBeDismissedAsyncronously = true;
    if (shouldBeDismissedAsyncronously)
    {
        this->dismissAsync();
    }
    else
    {
        this->exitModalState(0);
        this->fadeOut();
    }
}

void HelioCallout::handleCommandMessage(int commandId)
{
    if (commandId == CommandIDs::HideCallout)
    {
        this->exitModalState(0);
        this->fadeOut();
        //this->setVisible(false);
    }
}

void HelioCallout::dismissAsync()
{
    this->postCommandMessage(CommandIDs::HideCallout);
}

bool HelioCallout::keyPressed(const KeyPress &key)
{
    if (key.isKeyCode(KeyPress::escapeKey))
    {
        // give a chance to hosted component to react to escape key:
        this->contentComponent.postCommandMessage(CommandIDs::Cancel);
        this->inputAttemptWhenModal();
        return true;
    }
    
    return true;
}

void HelioCallout::findTargetPointAndUpdateBounds()
{
    if (this->alignsToMouse)
    {
        const Rectangle<int> b = App::Layout().getBounds();
        Rectangle<int> clickBounds(0, 0, 0, 0);
        clickBounds.setPosition(int(b.getWidth() * this->clickPointAbs.getX()),
                                int(b.getHeight() * this->clickPointAbs.getY()));
        
        const Rectangle<int> pageBounds = App::Layout().getBoundsForPopups();
        const Rectangle<int> pointBounds = clickBounds.expanded(CALLOUT_FRAME_MARGIN).constrainedWithin(pageBounds);
        
        this->pointToAndFit(pointBounds, pageBounds);
    }
    else
    {
        Point<int> positionInWorkspace = App::Layout().getLocalPoint(this->targetComponent, Point<int>(0, 0));
        Rectangle<int> topLevelBounds(positionInWorkspace.x,
                                      positionInWorkspace.y,
                                      this->targetComponent->getWidth(),
                                      this->targetComponent->getHeight());
        
        const Rectangle<int> pageBounds = App::Layout().getBoundsForPopups();
        const Rectangle<int> pointBounds = topLevelBounds.expanded(CALLOUT_FRAME_MARGIN);
        
        this->pointToAndFit(pointBounds, pageBounds);
    }
}

void HelioCallout::pointToAndFit(const Rectangle<int> &newAreaToPointTo,
                                 const Rectangle<int> &newAreaToFitIn)
{
    this->lastGoodTargetArea = newAreaToPointTo;
    this->lastGoodAvailableArea = newAreaToFitIn;
    
    const int borderSpace = this->getBorderSize();

    Rectangle<int> newBounds(this->contentComponent.getWidth() + borderSpace * 2,
                             this->contentComponent.getHeight() + borderSpace * 2);

    const int hw = (newBounds.getWidth() / 2);
    const int hh = (newBounds.getHeight() / 2);
    const float hwReduced = static_cast<float>(hw - borderSpace * 2);
    const float hhReduced = static_cast<float>(hh - borderSpace * 2);
    const float arrowIndent = borderSpace - arrowSize;
    
    Point<float> targets[4] =
    {
        Point<float>(static_cast<float>(newAreaToPointTo.getCentreX()), static_cast<float>(newAreaToPointTo.getBottom())),
        Point<float>(static_cast<float>(newAreaToPointTo.getRight()),   static_cast<float>(newAreaToPointTo.getCentreY())),
        Point<float>(static_cast<float>(newAreaToPointTo.getX()),       static_cast<float>(newAreaToPointTo.getCentreY())),
        Point<float>(static_cast<float>(newAreaToPointTo.getCentreX()), static_cast<float>(newAreaToPointTo.getY()))
    };
    
    Line<float> lines[4] =
    {
        Line<float>(targets[0].translated(-hwReduced, hh - arrowIndent),    targets[0].translated(hwReduced, hh - arrowIndent)),
        Line<float>(targets[1].translated(hw - arrowIndent, -hhReduced),    targets[1].translated(hw - arrowIndent, hhReduced)),
        Line<float>(targets[2].translated(-(hw - arrowIndent), -hhReduced), targets[2].translated(-(hw - arrowIndent), hhReduced)),
        Line<float>(targets[3].translated(-hwReduced, -(hh - arrowIndent)), targets[3].translated(hwReduced, -(hh - arrowIndent)))
    };
    
    const Rectangle<float> centrePointArea(newAreaToFitIn.reduced(hw, hh).toFloat());
    const Point<float> targetCentre(newAreaToPointTo.getCentre().toFloat());
    
    float nearest = 1.0e9f;
    
    for (int i = 0; i < 4; ++i)
    {
        Line<float> constrainedLine(centrePointArea.getConstrainedPoint(lines[i].getStart()),
                                    centrePointArea.getConstrainedPoint(lines[i].getEnd()));
        
        const Point<float> centre(constrainedLine.findNearestPointTo(targetCentre));
        float distanceFromCentre = centre.getDistanceFrom(targets[i]);
        
        if (! centrePointArea.intersects(lines[i]))
        {
            distanceFromCentre += 1000.f;
        }
        
        if (distanceFromCentre < nearest)
        {
            nearest = distanceFromCentre;
            
            this->targetPoint = targets[i];
            newBounds.setPosition(static_cast<int>(centre.x - hw),
                                  static_cast<int>(centre.y - hh));
        }
    }
    
    this->setBounds(newBounds);
}

void HelioCallout::updateShape()
{
    this->repaint();
    this->outline.clear();
    
    const float innerBorderPadding = 3.f;
    const auto bodyArea = this->contentComponent.getBounds()
        .toFloat().expanded(innerBorderPadding, innerBorderPadding);

    const auto maximumArea = this->getLocalBounds().toFloat();
    const auto arrowTip = this->targetPoint - this->getPosition().toFloat();
    //arrowTip.setX(jmin(jmax(arrowTip.getX(), maximumArea.getX()), maximumArea.getRight()));
    //arrowTip.setY(jmin(jmax(arrowTip.getY(), maximumArea.getY()), maximumArea.getBottom()));
    
    this->outline.addBubble(bodyArea,
        maximumArea, arrowTip, 1.f, this->arrowSize * 0.75f);
}
