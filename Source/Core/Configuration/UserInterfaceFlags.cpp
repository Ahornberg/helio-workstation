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
#include "SerializationKeys.h"
#include "Config.h"

//===----------------------------------------------------------------------===//
// Global UI state
//===----------------------------------------------------------------------===//

bool UserInterfaceFlags::isScalesHighlightingEnabled() const noexcept
{
    return this->scalesHighlighting;
}

void UserInterfaceFlags::setScalesHighlightingEnabled(bool enabled)
{
    if (this->scalesHighlighting == enabled)
    {
        return;
    }

    this->scalesHighlighting = enabled;
    this->listeners.call(&Listener::onScalesHighlightingFlagChanged, this->scalesHighlighting);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

bool UserInterfaceFlags::isNoteNameGuidesEnabled() const noexcept
{
    return this->noteNameGuides;
}

void UserInterfaceFlags::setNoteNameGuidesEnabled(bool enabled)
{
    if (this->noteNameGuides == enabled)
    {
        return;
    }

    this->noteNameGuides = enabled;
    this->listeners.call(&Listener::onNoteNameGuidesFlagChanged, this->noteNameGuides);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

bool UserInterfaceFlags::isOpenGlRendererEnabled() const noexcept
{
    return this->useOpenGLRenderer;
}

void UserInterfaceFlags::setOpenGlRendererEnabled(bool enabled)
{
    if (this->useOpenGLRenderer == enabled)
    {
        return;
    }

    this->useOpenGLRenderer = enabled;
    this->listeners.call(&Listener::onOpenGlRendererFlagChanged, this->useOpenGLRenderer);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);

}

bool UserInterfaceFlags::isNativeTitleBarEnabled() const noexcept
{
    return this->useNativeTitleBar;
}

void UserInterfaceFlags::setNativeTitleBarEnabled(bool enabled)
{
    if (this->useNativeTitleBar == enabled)
    {
        return;
    }

    this->useNativeTitleBar = enabled;
    this->listeners.call(&Listener::onNativeTitleBarFlagChanged, this->useNativeTitleBar);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

bool UserInterfaceFlags::isVelocityMapVisible() const noexcept
{
    return this->velocityMapVisible;
}

void UserInterfaceFlags::setVelocityMapVisible(bool visible)
{
    if (this->velocityMapVisible == visible)
    {
        return;
    }

    this->velocityMapVisible = visible;
    this->listeners.call(&Listener::onVelocityMapVisibilityFlagChanged, this->velocityMapVisible);
    // not saving this flag
}

void UserInterfaceFlags::toggleVelocityMapVisibility()
{
    this->setVelocityMapVisible(!this->velocityMapVisible);
}

bool UserInterfaceFlags::isFullProjectMapVisible() const noexcept
{
    return this->fullProjectMapVisible;
}

void UserInterfaceFlags::setFullProjectMapVisible(bool visible)
{
    if (this->fullProjectMapVisible == visible)
    {
        return;
    }

    this->fullProjectMapVisible = visible;
    this->listeners.call(&Listener::onProjectMapVisibilityFlagChanged, this->fullProjectMapVisible);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

void UserInterfaceFlags::toggleFullProjectMapVisibility()
{
    this->setFullProjectMapVisible(!this->fullProjectMapVisible);
}

bool UserInterfaceFlags::areExperimentalFeaturesEnabled() const noexcept
{
    return this->experimentalFeaturesOn;
}

bool UserInterfaceFlags::areUiAnimationsEnabled() const noexcept
{
    return this->rollAnimationsEnabled;
}

void UserInterfaceFlags::setUiAnimationsEnabled(bool enabled)
{
    if (this->rollAnimationsEnabled == enabled)
    {
        return;
    }

    this->rollAnimationsEnabled = enabled;
    this->listeners.call(&Listener::onUiAnimationsFlagChanged, this->rollAnimationsEnabled);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

void UserInterfaceFlags::setMouseWheelUsePanningByDefault(bool usePanning)
{
    if (this->mouseWheelFlags.usePanningByDefault == usePanning)
    {
        return;
    }

    this->mouseWheelFlags.usePanningByDefault = usePanning;
    this->listeners.call(&Listener::onMouseWheelFlagsChanged, this->mouseWheelFlags);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

void UserInterfaceFlags::setMouseWheelUseVerticalPanningByDefault(bool useVerticalPanning)
{
    if (this->mouseWheelFlags.useVerticalPanningByDefault == useVerticalPanning)
    {
        return;
    }

    this->mouseWheelFlags.useVerticalPanningByDefault = useVerticalPanning;
    this->listeners.call(&Listener::onMouseWheelFlagsChanged, this->mouseWheelFlags);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

void UserInterfaceFlags::setMouseWheelUseVerticalZoomingByDefault(bool useVerticalZooming)
{
    if (this->mouseWheelFlags.useVerticalZoomingByDefault == useVerticalZooming)
    {
        return;
    }

    this->mouseWheelFlags.useVerticalZoomingByDefault = useVerticalZooming;
    this->listeners.call(&Listener::onMouseWheelFlagsChanged, this->mouseWheelFlags);
    this->startTimer(UserInterfaceFlags::saveTimeoutMs);
}

UserInterfaceFlags::MouseWheelFlags UserInterfaceFlags::getMouseWheelFlags() const noexcept
{
    return this->mouseWheelFlags;
}

//===----------------------------------------------------------------------===//
// Serializable
//===----------------------------------------------------------------------===//

SerializedData UserInterfaceFlags::serialize() const
{
    using namespace Serialization;
    SerializedData tree(UI::Flags::uiFlags);
    
    tree.setProperty(UI::Flags::noteNameGuides, this->noteNameGuides);
    tree.setProperty(UI::Flags::scalesHighlighting, this->scalesHighlighting);
    tree.setProperty(UI::Flags::openGlRenderer, this->useOpenGLRenderer);
    tree.setProperty(UI::Flags::nativeTitleBar, this->useNativeTitleBar);
    tree.setProperty(UI::Flags::animations, this->rollAnimationsEnabled);
    tree.setProperty(UI::Flags::showFullProjectMap, this->fullProjectMapVisible);

    tree.setProperty(UI::Flags::mouseWheelAltMode, this->mouseWheelFlags.usePanningByDefault);
    tree.setProperty(UI::Flags::mouseWheelVerticalPanningByDefault, this->mouseWheelFlags.useVerticalPanningByDefault);
    tree.setProperty(UI::Flags::mouseWheelVerticalZoomingByDefault, this->mouseWheelFlags.useVerticalZoomingByDefault);
    // skips experimentalFeaturesOn, it's read only

    return tree;
}

void UserInterfaceFlags::deserialize(const SerializedData &data)
{
    using namespace Serialization;

    const auto root = data.hasType(UI::Flags::uiFlags) ?
        data : data.getChildWithName(UI::Flags::uiFlags);

    if (!root.isValid())
    {
        return;
    }

    this->noteNameGuides = root.getProperty(UI::Flags::noteNameGuides, this->noteNameGuides);
    this->scalesHighlighting = root.getProperty(UI::Flags::scalesHighlighting, this->scalesHighlighting);
    this->useOpenGLRenderer = root.getProperty(UI::Flags::openGlRenderer, this->useOpenGLRenderer);
    this->useNativeTitleBar = root.getProperty(UI::Flags::nativeTitleBar, this->useNativeTitleBar);
    this->rollAnimationsEnabled = root.getProperty(UI::Flags::animations, this->rollAnimationsEnabled);
    this->fullProjectMapVisible = root.getProperty(UI::Flags::showFullProjectMap, this->fullProjectMapVisible);

    this->mouseWheelFlags.usePanningByDefault =
        root.getProperty(UI::Flags::mouseWheelAltMode, this->mouseWheelFlags.usePanningByDefault);

    // todo remove this key in future versions
    const auto legacyAltDirection = root.getProperty(UI::Flags::mouseWheelAltDirection, false);
    this->mouseWheelFlags.useVerticalPanningByDefault =
        root.getProperty(UI::Flags::mouseWheelVerticalPanningByDefault, legacyAltDirection);
    this->mouseWheelFlags.useVerticalZoomingByDefault =
        root.getProperty(UI::Flags::mouseWheelVerticalZoomingByDefault, legacyAltDirection);

    this->velocityMapVisible = false; // not serializing that

    this->experimentalFeaturesOn = root.getProperty(UI::Flags::experimentalFeaturesOn, this->experimentalFeaturesOn);
}

void UserInterfaceFlags::reset() {}

//===----------------------------------------------------------------------===//
// Delayed save callback
//===----------------------------------------------------------------------===//

void UserInterfaceFlags::timerCallback()
{
    this->stopTimer();
    App::Config().save(this, Serialization::Config::activeUiFlags);
}

//===----------------------------------------------------------------------===//
// Flag listeners
//===----------------------------------------------------------------------===//

void UserInterfaceFlags::addListener(Listener *listener)
{
    jassert(MessageManager::getInstance()->currentThreadHasLockedMessageManager());
    this->listeners.add(listener);
}

void UserInterfaceFlags::removeListener(Listener *listener)
{
    jassert(MessageManager::getInstance()->currentThreadHasLockedMessageManager());
    this->listeners.remove(listener);
}

void UserInterfaceFlags::removeAllListeners()
{
    jassert(MessageManager::getInstance()->currentThreadHasLockedMessageManager());
    this->listeners.clear();
}
