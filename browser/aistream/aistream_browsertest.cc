/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <set>
#include <string>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/strings/strcat.h"
#include "base/synchronization/lock.h"
#include "base/test/run_until.h"
#include "brave/browser/aistream/aistream_service.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "url/gurl.h"

namespace aistream {

namespace {

constexpr char kToken[] = "test-token";

// A bridge (tools/aistream-bridge) that answers from what the test set. The
// event stream sends the current event and closes, so the browser reconnects
// every few seconds and a new event arrives whenever the test sets one.
class FakeBridge {
 public:
  std::unique_ptr<net::test_server::HttpResponse> Handle(
      const net::test_server::HttpRequest& request) {
    base::AutoLock lock(lock_);
    paths_seen_.insert(request.relative_url);
    if (request.relative_url == "/events") {
      ++events_requests_;
    }
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    auto authorization =
        request.headers.find(net::HttpRequestHeaders::kAuthorization);
    if (authorization == request.headers.end() ||
        authorization->second != base::StrCat({"Bearer ", kToken})) {
      response->set_code(net::HTTP_UNAUTHORIZED);
      return response;
    }
    if (request.relative_url == "/widgets") {
      response->set_content_type("application/json");
      response->set_content(base::StrCat(
          {"[", asked_.empty() ? "" : kAnswer, asked_.empty() ? "" : ",",
           R"({"id": "note", "title": "Note",)"
           R"( "markup": "<p id=\"start\">start</p>"}])"}));
    } else if (request.relative_url == "/ask") {
      asked_ = request.content;
      response->set_content_type("application/json");
      response->set_content(kAnswer);
    } else if (request.relative_url == "/events") {
      response->set_content_type("text/event-stream");
      response->set_content(event_.empty()
                                ? std::string(": open\n\n")
                                : base::StrCat({"data: ", event_, "\n\n"}));
    } else if (request.relative_url == "/widgets/note/message") {
      last_message_ = request.content;
      response->set_content_type("application/json");
      response->set_content(R"({"markup": "<p id=\"answer\">answer</p>"})");
    } else {
      response->set_code(net::HTTP_NOT_FOUND);
    }
    return response;
  }

  void SetEvent(const std::string& event) {
    base::AutoLock lock(lock_);
    event_ = event;
  }
  bool Saw(const std::string& path) {
    base::AutoLock lock(lock_);
    return paths_seen_.contains(path);
  }
  int events_requests() {
    base::AutoLock lock(lock_);
    return events_requests_;
  }
  std::string last_message() {
    base::AutoLock lock(lock_);
    return last_message_;
  }
  std::string asked() {
    base::AutoLock lock(lock_);
    return asked_;
  }

 private:
  static constexpr char kAnswer[] =
      R"({"id": "ask-1", "title": "why?",)"
      R"( "markup": "<p id=\"because\">because</p>"})";

  base::Lock lock_;
  std::string asked_;
  std::string event_;
  std::string last_message_;
  int events_requests_ = 0;
  std::set<std::string> paths_seen_;
};

}  // namespace

class AistreamBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    bridge_server_.RegisterRequestHandler(base::BindRepeating(
        &FakeBridge::Handle, base::Unretained(&bridge_)));
    ASSERT_TRUE(bridge_server_.InitializeAndListen());
    command_line->AppendSwitchASCII(AistreamService::kBridgeSwitch,
                                    bridge_server_.base_url().spec());
    base::Environment::Create()->SetVar(AistreamService::kTokenVariable,
                                        kToken);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    bridge_server_.StartAcceptingConnections();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  content::RenderFrameHost* WidgetFrame() {
    for (content::RenderFrameHost* frame :
         content::CollectAllRenderFrameHosts(web_contents())) {
      if (frame->GetLastCommittedURL().host() == "aistream-widget") {
        return frame;
      }
    }
    return nullptr;
  }

  // True once the widget frame has an element matching `selector`.
  bool WaitForFrameElement(const std::string& selector) {
    return base::test::RunUntil([&] {
      content::RenderFrameHost* frame = WidgetFrame();
      return frame &&
             content::EvalJs(frame, base::StrCat({"!!document.querySelector('",
                                                  selector, "')"}))
                     .ExtractBool();
    });
  }

  void OpenNewTab() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), GURL(chrome::kChromeUINewTabURL)));
  }

  std::string LeakUrl(const std::string& path) {
    return bridge_server_.GetURL(path).spec();
  }

  FakeBridge bridge_;
  net::EmbeddedTestServer bridge_server_;
};

IN_PROC_BROWSER_TEST_F(AistreamBrowserTest, AWidgetShowsItsMarkupInTheFrame) {
  OpenNewTab();
  ASSERT_TRUE(WaitForFrameElement("#start"));
  EXPECT_EQ(content::EvalJs(web_contents(),
                            "document.querySelector('[data-widget-id=note] "
                            "h4').textContent"),
            "Note");
}

IN_PROC_BROWSER_TEST_F(AistreamBrowserTest, AnEventChangesTheWidget) {
  OpenNewTab();
  ASSERT_TRUE(WaitForFrameElement("#start"));
  bridge_.SetEvent(
      R"({"widget": "note", "markup": "<p id=\"pushed\">pushed</p>"})");
  EXPECT_TRUE(WaitForFrameElement("#pushed"));
}

IN_PROC_BROWSER_TEST_F(AistreamBrowserTest,
                       AMessageCarriesThePageAndShowsTheAnswer) {
  OpenNewTab();
  ASSERT_TRUE(WaitForFrameElement("#start"));
  ASSERT_TRUE(content::ExecJs(web_contents(), R"(
    const input = document.querySelector('[data-widget-id=note] input');
    Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value')
        .set.call(input, 'hello');
    input.dispatchEvent(new Event('input', {bubbles: true}));
    document.querySelector('[data-widget-id=note] form').requestSubmit();
  )"));
  ASSERT_TRUE(WaitForFrameElement("#answer"));
  std::string body = bridge_.last_message();
  EXPECT_NE(body.find("\"message\":\"hello\""), std::string::npos) << body;
  // The DOM snapshot is the New Tab Page itself.
  EXPECT_NE(body.find("aistream-widgets"), std::string::npos);
}

IN_PROC_BROWSER_TEST_F(AistreamBrowserTest, TheMarkupRunsNoScriptAndLoadsNothing) {
  OpenNewTab();
  ASSERT_TRUE(WaitForFrameElement("#start"));
  // Escaped once for JSON and once more inside the C++ raw string: the
  // markup the frame gets is plain HTML with double quotes.
  std::string markup = base::StrCat(
      {"<script>window.ran = 'script'</script>"
       "<img src=\\\"",
       LeakUrl("/leak-img"),
       "\\\" onerror=\\\"window.ran = 'handler'\\\">"
       "<meta http-equiv=\\\"refresh\\\" content=\\\"0;url=",
       LeakUrl("/leak-refresh"),
       "\\\"><div style=\\\"background:url(",
       LeakUrl("/leak-css"),
       ")\\\">styled</div><p id=\\\"hostile\\\">hostile</p>"});
  bridge_.SetEvent(
      base::StrCat({R"({"widget": "note", "markup": ")", markup, "\"}"}));
  ASSERT_TRUE(WaitForFrameElement("#hostile"));

  content::RenderFrameHost* frame = WidgetFrame();
  ASSERT_TRUE(frame);
  EXPECT_EQ(content::EvalJs(frame, "String(window.ran)"), "undefined");
  EXPECT_EQ(content::EvalJs(frame, "document.body.querySelectorAll('script, "
                                   "meta').length"),
            0)
      << "the markup's script or meta survived";
  EXPECT_EQ(content::EvalJs(frame,
                            base::StrCat({"fetch('", LeakUrl("/leak-fetch"),
                                          "').then(() => 'loaded', () => "
                                          "'blocked')"})),
            "blocked");

  // Two more reconnects give anything that slipped through time to reach
  // the bridge before it is asked.
  int streams = bridge_.events_requests();
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return bridge_.events_requests() >= streams + 2; }));
  for (const char* path :
       {"/leak-img", "/leak-refresh", "/leak-css", "/leak-fetch"}) {
    EXPECT_FALSE(bridge_.Saw(path)) << path << " reached the network";
  }
  EXPECT_TRUE(WidgetFrame()) << "the frame navigated away";
}

IN_PROC_BROWSER_TEST_F(AistreamBrowserTest,
                       AQuestionFromTheSearchBoxBecomesAWidget) {
  OpenNewTab();
  ASSERT_TRUE(WaitForFrameElement("#start"));
  ASSERT_TRUE(base::test::RunUntil([&] {
    return content::EvalJs(web_contents(),
                           "!!document.querySelector('.input-container input')")
        .ExtractBool();
  }));
  ASSERT_TRUE(content::ExecJs(web_contents(), R"(
    const input = document.querySelector('.input-container input');
    input.focus();
    Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value')
        .set.call(input, 'why?');
    input.dispatchEvent(new Event('input', {bubbles: true}));
  )"));
  ASSERT_TRUE(base::test::RunUntil([&] {
    return content::EvalJs(
               web_contents(),
               "!!document.querySelector('[data-testid=aistream-ask]')")
        .ExtractBool();
  })) << "the search box offers no question for the agent";
  ASSERT_TRUE(content::ExecJs(
      web_contents(),
      "document.querySelector('[data-testid=aistream-ask]').click()"));

  ASSERT_TRUE(base::test::RunUntil([&] {
    return content::EvalJs(web_contents(),
                           "!!document.querySelector('[data-widget-id=ask-1]')")
        .ExtractBool();
  })) << "the answer did not become a widget";
  std::string body = bridge_.asked();
  EXPECT_NE(body.find("\"query\":\"why?\""), std::string::npos) << body;
  EXPECT_NE(body.find("aistream-widgets"), std::string::npos)
      << "the question did not carry the page";
  // The newest answer comes first, before the widgets the bridge started
  // with.
  EXPECT_EQ(content::EvalJs(web_contents(),
                            "[...document.querySelectorAll('[data-widget-id]')]"
                            ".map(e => e.dataset.widgetId).join()"),
            "ask-1,note");
  EXPECT_EQ(content::EvalJs(web_contents(),
                            "document.querySelector('[data-widget-id=ask-1] "
                            "h4').textContent"),
            "why?");
}

}  // namespace aistream
