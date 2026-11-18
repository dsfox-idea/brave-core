/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AISTREAM_AISTREAM_PAGE_HANDLER_H_
#define BRAVE_BROWSER_AISTREAM_AISTREAM_PAGE_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/aistream/aistream.mojom.h"
#include "brave/browser/aistream/aistream_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace aistream {

// growser#422: one per New Tab Page. Without a service (no --aistream-bridge)
// it lists no widgets, so the page draws none.
class AistreamPageHandler : public mojom::WidgetHandler,
                            public AistreamService::Observer {
 public:
  AistreamPageHandler(mojo::PendingReceiver<mojom::WidgetHandler> receiver,
                      AistreamService* service);
  AistreamPageHandler(const AistreamPageHandler&) = delete;
  AistreamPageHandler& operator=(const AistreamPageHandler&) = delete;
  ~AistreamPageHandler() override;

  // mojom::WidgetHandler:
  void SetPage(mojo::PendingRemote<mojom::WidgetPage> page) override;
  void GetWidgets(GetWidgetsCallback callback) override;
  void SendMessage(const std::string& widget_id,
                   const std::string& message,
                   const std::string& dom,
                   SendMessageCallback callback) override;

  // AistreamService::Observer:
  void OnWidgetMarkup(const std::string& widget_id,
                      const std::string& markup) override;

 private:
  mojo::Receiver<mojom::WidgetHandler> receiver_;
  mojo::Remote<mojom::WidgetPage> page_;
  const raw_ptr<AistreamService> service_;
  base::ScopedObservation<AistreamService, AistreamService::Observer>
      observation_{this};
};

}  // namespace aistream

#endif  // BRAVE_BROWSER_AISTREAM_AISTREAM_PAGE_HANDLER_H_
