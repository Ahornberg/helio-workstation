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

class ProjectInfo;
class VersionControlEditor;

#include "TreeItem.h"
#include "TrackedItemsSource.h"
#include "SafeTreeItemPointer.h"

#include "ProjectListener.h"

#include "Delta.h"
#include "Revision.h"
#include "Head.h"
#include "Pack.h"
#include "StashesRepository.h"

class VersionControl final :
    public Serializable,
    public ChangeListener,
    public ChangeBroadcaster
{
public:

    explicit VersionControl(VCS::TrackedItemsSource &parent);
    ~VersionControl() override;
    
    //===------------------------------------------------------------------===//
    // VCS
    //===------------------------------------------------------------------===//

    VersionControlEditor *createEditor();
    VCS::Head &getHead() { return this->head; }
    VCS::Revision::Ptr getRoot() { return this->rootRevision; }

    void moveHead(const VCS::Revision::Ptr revision);
    void checkout(const VCS::Revision::Ptr revision);
    void cherryPick(const VCS::Revision::Ptr revision, const Array<Uuid> uuids);

    void appendSubtree(const VCS::Revision::Ptr subtree, const String &appendRevisionId);
    VCS::Revision::Ptr updateShallowRevisionData(const String &id, const ValueTree &data);

    bool resetChanges(SparseSet<int> selectedItems);
    bool resetAllChanges();
    bool commit(SparseSet<int> selectedItems, const String &message);
    void quickAmendItem(VCS::TrackedItem *targetItem); // for first commit

    bool stash(SparseSet<int> selectedItems, const String &message, bool shouldKeepChanges = false);
    bool applyStash(const VCS::Revision::Ptr stash, bool shouldKeepStash = false);
    bool applyStash(const String &stashId, bool shouldKeepStash = false);
    
    bool hasQuickStash() const;
    bool quickStashAll();
    bool applyQuickStash();

    //===------------------------------------------------------------------===//
    // Network
    //===------------------------------------------------------------------===//

    void syncProject();

    //===------------------------------------------------------------------===//
    // Serializable
    //===------------------------------------------------------------------===//

    ValueTree serialize() const override;
    void deserialize(const ValueTree &tree) override;
    void reset() override;

    //===------------------------------------------------------------------===//
    // ChangeListener
    //===------------------------------------------------------------------===//

    void changeListenerCallback(ChangeBroadcaster *source) override;
    
protected:

    VCS::Revision::Ptr getRevisionById(const VCS::Revision::Ptr startFrom, const String &id) const;

    VCS::Pack::Ptr pack;
    VCS::Head head;
    VCS::StashesRepository::Ptr stashes;
    VCS::Revision::Ptr rootRevision; // the history tree itself

private:

    VCS::TrackedItemsSource &parent;

    
    // TODO
    //void updateSyncCache(const String &revisionId, Time syncedAt);
    //SparseHashMap<String, uint64, StringHash> remoteSyncCache;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VersionControl)
    JUCE_DECLARE_WEAK_REFERENCEABLE(VersionControl)
};
