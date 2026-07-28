// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_

#include <string>

#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace ai_chat {

// Move this function to browser_finder.
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/side_panel/companion/companion_side_panel_controller_utils.h;l=19;drc=e87fd2634a1140a87d59c7aa72668d16e4c102c2
Browser* GetBrowserForWebContents(content::WebContents* web_contents);
void ClosePanel(content::WebContents* web_contents);

// Forward (tab -> side panel): when `ai_chat_web_contents` is a full browser
// tab and the `kAIChatMoveFullPageToSidePanel` feature is enabled, moves the
// live AI Chat `WebContents` into the side panel (preserving state). Returns
// true if the conversation was moved. Opening the link that prompted the move
// is the caller's separate responsibility - see `AIChatFullPageLinkObserver`
// for anchor clicks, which the browser opens itself, and
// `AIChatUIPageHandler::OpenURLInNewTab` for links AI Chat opens over Mojo.
// Desktop only; the definition lives in the toolkit_views translation unit.
bool MaybeMoveFullPageChatToSidePanel(
    content::WebContents* ai_chat_web_contents);

// Reverse (side panel -> tab): when `ai_chat_web_contents` is the live AI Chat
// conversation hosted in the global side panel, moves that live `WebContents`
// into a new full-page tab (preserving state) and closes the panel. Returns
// true if the switch was handled this way (the caller must then NOT open a
// fresh full-page tab). No-op (returns false) unless the
// `kAIChatMoveFullPageToSidePanel` feature is enabled, the side panel is the
// global standalone AI Chat, and it is the one being shown. Desktop only; the
// definition lives in the toolkit_views translation unit.
bool MaybeMoveSidePanelChatToTab(content::WebContents* ai_chat_web_contents);

// Closes the side panel only if the AI Chat entry is the one currently being
// shown. Used when moving a conversation to a full-page tab, where leaving the
// now-duplicate AI Chat side panel open would be confusing.
void ClosePanelIfChatActive(content::WebContents* web_contents);

// Returns true if the side panel should be global for all tabs in a tab strip,
// or false if it should be per-tab.
bool ShouldSidePanelBeGlobal(Profile* profile);

// Opens (or focuses) the global AI Chat side panel in the most recently active
// normal browser window for `profile` and shows the conversation identified by
// `conversation_uuid`. Two cases, matching how the panel is hosted:
//   * The panel is not currently hosting AI Chat (closed, or showing another
//     entry): show the `kChatUI` entry - which creates its `WebContents` - and
//     navigate that contents to the conversation URL.
//   * The panel is already hosting AI Chat: reuse the live `WebContents` and
//     navigate it to the conversation, preserving panel state (a no-op when the
//     requested conversation is already the one shown).
// No-op if `profile` is not in global side panel mode (see
// `ShouldSidePanelBeGlobal()`) or has no eligible browser window. This is the
// browser-layer backing for `ai_chat::AIChatService::OpenSidePanel()`. Desktop
// only; the real definition lives in the toolkit_views translation unit, with a
// NOTIMPLEMENTED stub otherwise.
void OpenConversationInSidePanel(Profile* profile,
                                 const std::string& conversation_uuid);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_
