/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "components/tabs/public/tab_strip_collection.h"

// To avoid enumeration values not handled in switch error.
#define CommandAddNote                  \
  CommandAddNote:                       \
  case CommandRestoreTab:               \
  case CommandBookmarkAllTabs:          \
  case CommandShowVerticalTabs:         \
  case CommandToggleTabMuted:           \
  case CommandBringAllTabsToThisWindow: \
  case CommandCloseDuplicateTabs:       \
  case CommandOpenInContainer:          \
  case CommandRenameTab

#define DraggingTabsSession DraggingTabsSessionChromium

// Pass additional parameter for contents_data_->AddTabRecursive().
#define AddTabRecursive(...)                                    \
  /* meaningless call to address "contents_data_->" */          \
  pinned_collection();                                          \
  /* passing nullptr as opener when creating a empty new tab */ \
  /* in order not to create a nested tree node */               \
  auto* opener = tab_model->opener_was_set_for_empty_new_tab()  \
                     ? nullptr                                  \
                     : tab_model->opener();                     \
  contents_data_->AddTabRecursive(__VA_ARGS__, opener)

// When adding a new tab with a "TYPED" transition, set the
// opener_was_set_for_empty_new_tab() flag to true to avoid creating a nested
// tree node. This is followed by inherit_opener = true; in the original code.
// The current condition is:
// * Page transition type includes ui::PAGE_TRANSITION_TYPED qualifier
// * The index is the last index in the tab strip (count())
#define BRAVE_TAB_STRIP_MODEL_ADD_TAB \
  tab->set_opener_was_set_for_empty_new_tab();

// Give the tree-tab delegate a chance to hoist any child of a moving tab's
// tree node that is not itself part of this batch, before any tab in the
// batch is detached, so a connected subtree that is moving together stays
// nested instead of being flattened by the per-tab detach path below.
#define BRAVE_DETACH_TABS_AND_COLLECTIONS_FOR_INSERTION_PREPARE_TREE \
  contents_data_->PrepareTreeTabNodesForBatchDetach(tab_interfaces);

// Detach an entire tree-tab subtree as one atomic unit when `tab_interface`
// is the topmost tab of a subtree that isn't already covered by a moving
// ancestor, mirroring how whole groups/splits are detached above.
// NOLINTBEGIN(readability/braces)
#define BRAVE_DETACH_TABS_AND_COLLECTIONS_FOR_INSERTION_TREE_NODE_BRANCH   \
  }                                                                        \
  else if (contents_data_->ShouldDetachAsTreeSubtreeRoot(                  \
               const_cast<tabs::TabInterface*>(tab_interface),             \
               tab_interfaces)) {                                          \
    owned_tabs_and_collections.emplace_back(DetachTreeTabNodeForInsertion( \
        const_cast<tabs::TabInterface*>(tab_interface)));
// NOLINTEND(readability/braces)

#include <chrome/browser/ui/tabs/tab_strip_model.cc>  // IWYU pragma: export

#undef BRAVE_DETACH_TABS_AND_COLLECTIONS_FOR_INSERTION_TREE_NODE_BRANCH
#undef BRAVE_DETACH_TABS_AND_COLLECTIONS_FOR_INSERTION_PREPARE_TREE
#undef BRAVE_TAB_STRIP_MODEL_ADD_TAB
#undef AddTabRecursive
#undef DraggingTabsSession
#undef CommandAddNote

// Defined here (rather than before the #include above) because these bodies
// use ReentrancyCheck/DetachTabCollectionImpl/InsertDetachedCollectionImpl/
// ConstrainInsertionIndex, which are declared inside the real
// tab_strip_model.cc textually included above.
std::unique_ptr<DetachedTabCollection>
TabStripModel::DetachTreeTabNodeForInsertion(
    tabs::TabInterface* subtree_root_tab) {
  ReentrancyCheck reentrancy_check(&reentrancy_guard_);

  auto* tree_node_collection =
      tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(
          subtree_root_tab);
  CHECK(tree_node_collection);

  std::optional<int> active_index_in_collection;
  int index = 0;
  for (tabs::TabInterface* tab : *tree_node_collection) {
    if (tab->IsActivated()) {
      active_index_in_collection = index;
      break;
    }
    index++;
  }

  // Unregister every node in this subtree from the source window's
  // TreeTabModel before it is physically removed.
  contents_data_->WillDetachTreeTabNodeSubtree(*tree_node_collection);

  std::unique_ptr<tabs::TabCollection> detached_collection =
      DetachTabCollectionImpl(
          tree_node_collection,
          base::BindOnce(&tabs::TabStripCollection::RemoveTabCollection,
                         base::Unretained(contents_data_.get()),
                         tree_node_collection),
          base::DoNothing());

  return std::make_unique<DetachedTabCollection>(
      base::WrapUnique(static_cast<tabs::TreeTabNodeTabCollection*>(
          detached_collection.release())),
      active_index_in_collection, /*pinned_=*/false);
}

gfx::Range TabStripModel::InsertDetachedTreeTabNodeAt(
    std::unique_ptr<DetachedTabCollection> tree_node,
    int index) {
  ReentrancyCheck reentrancy_check(&reentrancy_guard_);
  CHECK(std::holds_alternative<std::unique_ptr<tabs::TreeTabNodeTabCollection>>(
      tree_node->collection_));

  std::unique_ptr<tabs::TreeTabNodeTabCollection>
      tree_node_collection_unique_ptr =
          std::move(std::get<std::unique_ptr<tabs::TreeTabNodeTabCollection>>(
              tree_node->collection_));
  tabs::TreeTabNodeTabCollection* tree_node_collection =
      tree_node_collection_unique_ptr.get();

  // Notify tab is added to model.
  for (tabs::TabInterface* tab : *tree_node_collection) {
    static_cast<tabs::TabModel*>(tab)->OnAddedToModel(this);
  }

  index = ConstrainInsertionIndex(index, false);

  // Registers the subtree with the destination window's TreeTabModel (via
  // BraveTabStripCollection::InsertDetachedTreeTabNode ->
  // DidAttachTreeTabNodeSubtree) only after the physical insertion below,
  // since TreeTabModel::AddTreeTabNode needs the node's post-move parent
  // chain to compute the collapsed-ancestor cache correctly.
  return InsertDetachedCollectionImpl(
      tree_node_collection, tree_node->active_index_,
      base::BindOnce(&tabs::TabStripCollection::InsertDetachedTreeTabNode,
                     base::Unretained(contents_data_.get()),
                     std::move(tree_node_collection_unique_ptr), index),
      base::DoNothing());
}
