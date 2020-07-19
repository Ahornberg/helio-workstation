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

//[Headers]
#include "TreeNode.h"
#include "NavigationHistory.h"

class IconButton;
class HeadlineItem;
class HeadlineItemDataSource;

//[/Headers]

#include "HeadlineNavigationPanel.h"

class Headline final : public Component,
                       public AsyncUpdater
{
public:

    Headline();
    ~Headline();

    //[UserMethods]
    void syncWithTree(NavigationHistory &history, WeakReference<TreeNode> leaf);
    
    void showSelectionMenu(WeakReference<HeadlineItemDataSource> menuSource);
    void hideSelectionMenu();

    HeadlineItem *getTailItem() const noexcept;
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void handleCommandMessage (int commandId) override;


private:

    //[UserVariables]

    // A way to receive a single coalesced update from multiple signaling sub-items:
    void handleAsyncUpdate() override;
    int rebuildChain(WeakReference<TreeNode> leaf);
    int getChainWidth() const noexcept;

    ComponentAnimator animator;

    // A number of items associated with tree hierarchy
    OwnedArray<HeadlineItem> chain;

    // A special item for `current selection` menu
    // (if present, is always shown at the tail of chain)
    UniquePointer<HeadlineItem> selectionItem;

    float getAlphaForAnimation() const noexcept;

    static constexpr auto itemsOverlapOffset = 16;
    static constexpr auto rootNodeOffset = Globals::UI::sidebarWidth;

    //[/UserVariables]

    UniquePointer<HeadlineNavigationPanel> navPanel;
    UniquePointer<IconButton> consoleButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Headline)
};


