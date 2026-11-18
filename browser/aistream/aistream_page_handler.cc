/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/aistream/aistream_page_handler.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"

namespace aistream {

AistreamPageHandler::AistreamPageHandler(
    mojo::PendingReceiver<mojom::WidgetHandler> receiver,
    AistreamService* service)
    : receiver_(this, std::move(receiver)), service_(service) {
  if (service_) {
    observation_.Observe(service_);
  }
}

AistreamPageHandler::~AistreamPageHandler() = default;

void AistreamPageHandler::SetPage(
    mojo::PendingRemote<mojom::WidgetPage> page) {
  page_.reset();
  page_.Bind(std::move(page));
}

void AistreamPageHandler::GetWidgets(GetWidgetsCallback callback) {
  if (!service_) {
    std::move(callback).Run({});
    return;
  }
  service_->GetWidgets(base::BindOnce(
      [](GetWidgetsCallback callback, std::vector<Widget> widgets) {
        std::vector<mojom::WidgetPtr> result;
        for (auto& widget : widgets) {
          result.push_back(mojom::Widget::New(std::move(widget.id),
                                              std::move(widget.title),
                                              std::move(widget.markup)));
        }
        std::move(callback).Run(std::move(result));
      },
      std::move(callback)));
}

void AistreamPageHandler::SendMessage(const std::string& widget_id,
                                      const std::string& message,
                                      const std::string& dom,
                                      SendMessageCallback callback) {
  if (!service_) {
    std::move(callback).Run(std::string(), "widgets are off");
    return;
  }
  service_->SendMessage(
      widget_id, message, dom,
      base::BindOnce(
          [](SendMessageCallback callback,
             base::expected<std::string, std::string> reply) {
            if (reply.has_value()) {
              std::move(callback).Run(std::move(reply.value()), std::string());
            } else {
              std::move(callback).Run(std::string(), std::move(reply.error()));
            }
          },
          std::move(callback)));
}

void AistreamPageHandler::OnWidgetMarkup(const std::string& widget_id,
                                         const std::string& markup) {
  if (page_) {
    page_->OnWidgetMarkup(widget_id, markup);
  }
}

}  // namespace aistream
