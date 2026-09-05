/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/edge_hover_detector.h"

#include <memory>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/test/widget_test.h"

namespace {

constexpr base::TimeDelta kEnterDelay = EdgeHoverDetector::kMouseEnterDelay;
constexpr base::TimeDelta kExitDelay = EdgeHoverDetector::kMouseExitDelay;

ui::MouseEvent MakeMouseEvent(ui::EventType type) {
  return ui::MouseEvent(type, gfx::Point(), gfx::Point(),
                        base::TimeTicks::Now(), 0, 0);
}

class EdgeHoverDetectorTest : public views::test::WidgetTest {
 public:
  EdgeHoverDetectorTest()
      : views::test::WidgetTest(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  // Growser-185: the widget is a HOST here, never a surface anyone looks at -
  // every test drives the detector through DetectHoverState() or
  // HandleEventForTesting(), and IsHoverPoint() answers from a member rather
  // than from where the cursor is. It must therefore stay hidden, because
  // EdgeHoverDetector's constructor puts a REAL views::EventMonitor on this
  // window, and a shown window receives the real mouse.
  //
  // That is not a timing race - the clock here is mocked. It is one ambient
  // kMouseExited reaching OnEvent, which calls ApplyWithDelay(false) and stops
  // the pending enter timer, so MouseExitedEventStartsExitTimer failed its
  // ASSERT_TRUE(hovering()) on a machine where somebody moved the mouse. A bot
  // never does, which is why this survived until it ran on a desk.
  //
  // A window that was never shown gets no mouse messages on any of the three
  // platforms, so not showing it is the whole fix.
  void SetUp() override {
    WidgetTest::SetUp();
    widget_.reset(CreateTopLevelNativeWidget(
        views::Widget::InitParams::CLIENT_OWNS_WIDGET));
    widget_->SetSize(gfx::Size(200, 200));
    ASSERT_FALSE(widget_->IsVisible())
        << "showing this widget hands the detector the real mouse";
    detector_ = std::make_unique<EdgeHoverDetector>(
        widget_.get(),
        base::BindRepeating(&EdgeHoverDetectorTest::IsHoverPoint,
                            base::Unretained(this)),
        base::BindRepeating(&EdgeHoverDetectorTest::OnHoverStateChanged,
                            base::Unretained(this)));
  }

  void TearDown() override {
    detector_.reset();
    widget_.reset();
    WidgetTest::TearDown();
  }

 protected:
  bool IsHoverPoint(const gfx::Point&) { return target_hover_; }
  void OnHoverStateChanged(bool hovering) { changes_.push_back(hovering); }

  bool target_hover_ = false;
  std::vector<bool> changes_;
  std::unique_ptr<views::Widget> widget_;
  std::unique_ptr<EdgeHoverDetector> detector_;
};

TEST_F(EdgeHoverDetectorTest, InitialStateIsNotHovering) {
  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());
}

TEST_F(EdgeHoverDetectorTest, EnterCommitsAfterEnterDelay) {
  target_hover_ = true;
  detector_->DetectHoverState();
  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());

  task_environment()->FastForwardBy(kEnterDelay - base::Milliseconds(1));
  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());

  task_environment()->FastForwardBy(base::Milliseconds(1));
  EXPECT_TRUE(detector_->hovering());
  EXPECT_THAT(changes_, testing::ElementsAre(true));
}

TEST_F(EdgeHoverDetectorTest, ExitCommitsAfterExitDelay) {
  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(kEnterDelay);
  ASSERT_TRUE(detector_->hovering());
  ASSERT_EQ(1u, changes_.size());

  target_hover_ = false;
  detector_->DetectHoverState();
  EXPECT_TRUE(detector_->hovering());

  task_environment()->FastForwardBy(kExitDelay - base::Milliseconds(1));
  EXPECT_TRUE(detector_->hovering());

  task_environment()->FastForwardBy(base::Milliseconds(1));
  EXPECT_FALSE(detector_->hovering());
  EXPECT_THAT(changes_, testing::ElementsAre(true, false));
}

TEST_F(EdgeHoverDetectorTest, EnterAbortedBeforeCommitFiresNoCallback) {
  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Milliseconds(30));

  target_hover_ = false;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Seconds(1));

  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());
}

TEST_F(EdgeHoverDetectorTest, ExitAbortedBeforeCommitFiresNoCallback) {
  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(kEnterDelay);
  ASSERT_TRUE(detector_->hovering());
  ASSERT_EQ(1u, changes_.size());

  target_hover_ = false;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Milliseconds(100));

  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Seconds(1));

  EXPECT_TRUE(detector_->hovering());
  EXPECT_EQ(1u, changes_.size());
}

TEST_F(EdgeHoverDetectorTest, RedundantDetectDoesNothing) {
  target_hover_ = false;
  detector_->DetectHoverState();
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Seconds(1));

  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());
}

TEST_F(EdgeHoverDetectorTest, MouseMovedEventDrivesEnter) {
  target_hover_ = true;
  detector_->HandleEventForTesting(MakeMouseEvent(ui::EventType::kMouseMoved));
  task_environment()->FastForwardBy(kEnterDelay);

  EXPECT_TRUE(detector_->hovering());
  EXPECT_THAT(changes_, testing::ElementsAre(true));
}

TEST_F(EdgeHoverDetectorTest, MouseExitedEventStartsExitTimer) {
  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(kEnterDelay);
  ASSERT_TRUE(detector_->hovering());

  detector_->HandleEventForTesting(MakeMouseEvent(ui::EventType::kMouseExited));
  EXPECT_TRUE(detector_->hovering());

  task_environment()->FastForwardBy(kExitDelay);

  EXPECT_FALSE(detector_->hovering());
  EXPECT_THAT(changes_, testing::ElementsAre(true, false));
}

TEST_F(EdgeHoverDetectorTest, WidgetDestructionCancelsPendingTimer) {
  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Milliseconds(30));

  widget_.reset();

  task_environment()->FastForwardBy(base::Seconds(1));

  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());
}

TEST_F(EdgeHoverDetectorTest, DetectAfterWidgetDestroyedIsNoOp) {
  widget_.reset();

  target_hover_ = true;
  detector_->DetectHoverState();
  task_environment()->FastForwardBy(base::Seconds(1));

  EXPECT_FALSE(detector_->hovering());
  EXPECT_TRUE(changes_.empty());
}

}  // namespace
