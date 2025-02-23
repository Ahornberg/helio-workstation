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

#pragma once

#include "IconButton.h"
#include "HeadlineItemArrow.h"
#include "Workspace.h"
#include "ColourIDs.h"

class HeadlineNavigationPanel final : public Component
{
public:

    HeadlineNavigationPanel()
    {
        this->setPaintingIsUnclipped(true);
        this->setInterceptsMouseClicks(false, true);

        this->navigatePrevious = make<IconButton>(Icons::findByName(Icons::back, 20), CommandIDs::ShowPreviousPage);
        this->addAndMakeVisible(this->navigatePrevious.get());

        this->navigateNext = make<IconButton>(Icons::findByName(Icons::forward, 20), CommandIDs::ShowNextPage);
        this->addAndMakeVisible(this->navigateNext.get());

        this->arrow = make<HeadlineItemArrow>();
        this->addAndMakeVisible(this->arrow.get());

        this->updateState(false, false);

        this->setSize(64, Globals::UI::headlineHeight);
    }

    void updateState(bool canGoPrevious, bool canGoNext)
    {
        this->navigatePrevious->setInterceptsMouseClicks(canGoPrevious, false);
        this->navigatePrevious->setIconAlphaMultiplier(canGoPrevious ? 0.45f : 0.2f);
        this->navigateNext->setInterceptsMouseClicks(canGoNext, false);
        this->navigateNext->setIconAlphaMultiplier(canGoNext ? 0.45f : 0.2f);
    }

    void paint(Graphics &g) override
    {
        g.setColour(this->bgColour);
        g.fillRect(0, 0, this->getWidth() - 2, this->getHeight() - 2);
    }

    void resized() override
    {
        constexpr auto buttonWidth = 29;
        this->navigatePrevious->setBounds(0, 0, buttonWidth, this->getHeight() - 1);
        this->navigateNext->setBounds(20, 0, buttonWidth, this->getHeight() - 1);
        this->arrow->setBounds(this->getWidth() - HeadlineItemArrow::arrowWidth - 1,
            0, HeadlineItemArrow::arrowWidth, this->getHeight() - 1);
    }

    void handleCommandMessage(int commandId) override
    {
        switch (commandId)
        {
        case CommandIDs::ShowPreviousPage:
            App::Workspace().navigateBackwardIfPossible();
            break;
        case CommandIDs::ShowNextPage:
            App::Workspace().navigateForwardIfPossible();
            break;
        default:
            break;
        }
    }

private:

    const Colour bgColour = findDefaultColour(ColourIDs::BackgroundA::fill);

    UniquePointer<IconButton> navigatePrevious;
    UniquePointer<IconButton> navigateNext;
    UniquePointer<HeadlineItemArrow> arrow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeadlineNavigationPanel)
};
