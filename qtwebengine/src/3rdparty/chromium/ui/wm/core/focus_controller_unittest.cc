// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/focus_controller.h"

#include <map>

#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/event_generator.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tracker.h"
#include "ui/base/ime/dummy_text_input_client.h"
#include "ui/base/ime/text_input_focus_manager.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_handler.h"
#include "ui/wm/core/base_focus_rules.h"
#include "ui/wm/core/wm_state.h"
#include "ui/wm/public/activation_change_observer.h"
#include "ui/wm/public/activation_client.h"

namespace wm {

class FocusNotificationObserver : public aura::client::ActivationChangeObserver,
                                  public aura::client::FocusChangeObserver {
 public:
  FocusNotificationObserver()
      : activation_changed_count_(0),
        focus_changed_count_(0),
        reactivation_count_(0),
        reactivation_requested_window_(NULL),
        reactivation_actual_window_(NULL) {}
  virtual ~FocusNotificationObserver() {}

  void ExpectCounts(int activation_changed_count, int focus_changed_count) {
    EXPECT_EQ(activation_changed_count, activation_changed_count_);
    EXPECT_EQ(focus_changed_count, focus_changed_count_);
  }
  int reactivation_count() const {
    return reactivation_count_;
  }
  aura::Window* reactivation_requested_window() const {
    return reactivation_requested_window_;
  }
  aura::Window* reactivation_actual_window() const {
    return reactivation_actual_window_;
  }

 private:
  // Overridden from aura::client::ActivationChangeObserver:
  virtual void OnWindowActivated(aura::Window* gained_active,
                                 aura::Window* lost_active) OVERRIDE {
    ++activation_changed_count_;
  }
  virtual void OnAttemptToReactivateWindow(
      aura::Window* request_active,
      aura::Window* actual_active) OVERRIDE {
    ++reactivation_count_;
    reactivation_requested_window_ = request_active;
    reactivation_actual_window_ = actual_active;
  }

  // Overridden from aura::client::FocusChangeObserver:
  virtual void OnWindowFocused(aura::Window* gained_focus,
                               aura::Window* lost_focus) OVERRIDE {
    ++focus_changed_count_;
  }

  int activation_changed_count_;
  int focus_changed_count_;
  int reactivation_count_;
  aura::Window* reactivation_requested_window_;
  aura::Window* reactivation_actual_window_;

  DISALLOW_COPY_AND_ASSIGN(FocusNotificationObserver);
};

class WindowDeleter {
 public:
  virtual aura::Window* GetDeletedWindow() = 0;

 protected:
  virtual ~WindowDeleter() {}
};

// ActivationChangeObserver and FocusChangeObserver that keeps track of whether
// it was notified about activation changes or focus changes with a deleted
// window.
class RecordingActivationAndFocusChangeObserver
    : public aura::client::ActivationChangeObserver,
      public aura::client::FocusChangeObserver {
 public:
  RecordingActivationAndFocusChangeObserver(aura::Window* root,
                                            WindowDeleter* deleter)
      : root_(root),
        deleter_(deleter),
        was_notified_with_deleted_window_(false) {
    aura::client::GetActivationClient(root_)->AddObserver(this);
    aura::client::GetFocusClient(root_)->AddObserver(this);
  }
  virtual ~RecordingActivationAndFocusChangeObserver() {
    aura::client::GetActivationClient(root_)->RemoveObserver(this);
    aura::client::GetFocusClient(root_)->RemoveObserver(this);
  }

  bool was_notified_with_deleted_window() const {
    return was_notified_with_deleted_window_;
  }

  // Overridden from aura::client::ActivationChangeObserver:
  virtual void OnWindowActivated(aura::Window* gained_active,
                                 aura::Window* lost_active) OVERRIDE {
    if (lost_active && lost_active == deleter_->GetDeletedWindow())
      was_notified_with_deleted_window_ = true;
  }

  // Overridden from aura::client::FocusChangeObserver:
  virtual void OnWindowFocused(aura::Window* gained_focus,
                               aura::Window* lost_focus) OVERRIDE {
    if (lost_focus && lost_focus == deleter_->GetDeletedWindow())
      was_notified_with_deleted_window_ = true;
  }

 private:
  aura::Window* root_;

  // Not owned.
  WindowDeleter* deleter_;

  // Whether the observer was notified about the loss of activation or the
  // loss of focus with a window already deleted by |deleter_| as the
  // |lost_active| or |lost_focus| parameter.
  bool was_notified_with_deleted_window_;

  DISALLOW_COPY_AND_ASSIGN(RecordingActivationAndFocusChangeObserver);
};

// ActivationChangeObserver that deletes the window losing activation.
class DeleteOnLoseActivationChangeObserver :
    public aura::client::ActivationChangeObserver,
    public WindowDeleter {
 public:
  explicit DeleteOnLoseActivationChangeObserver(aura::Window* window)
      : root_(window->GetRootWindow()),
        window_(window),
        did_delete_(false) {
    aura::client::GetActivationClient(root_)->AddObserver(this);
  }
  virtual ~DeleteOnLoseActivationChangeObserver() {
    aura::client::GetActivationClient(root_)->RemoveObserver(this);
  }

  // Overridden from aura::client::ActivationChangeObserver:
  virtual void OnWindowActivated(aura::Window* gained_active,
                                 aura::Window* lost_active) OVERRIDE {
    if (window_ && lost_active == window_) {
      delete lost_active;
      did_delete_ = true;
    }
  }

  // Overridden from WindowDeleter:
  virtual aura::Window* GetDeletedWindow() OVERRIDE {
    return did_delete_ ? window_ : NULL;
  }

 private:
  aura::Window* root_;
  aura::Window* window_;
  bool did_delete_;

  DISALLOW_COPY_AND_ASSIGN(DeleteOnLoseActivationChangeObserver);
};

// FocusChangeObserver that deletes the window losing focus.
class DeleteOnLoseFocusChangeObserver
    : public aura::client::FocusChangeObserver,
      public WindowDeleter {
 public:
  explicit DeleteOnLoseFocusChangeObserver(aura::Window* window)
      : root_(window->GetRootWindow()),
        window_(window),
        did_delete_(false) {
    aura::client::GetFocusClient(root_)->AddObserver(this);
  }
  virtual ~DeleteOnLoseFocusChangeObserver() {
    aura::client::GetFocusClient(root_)->RemoveObserver(this);
  }

  // Overridden from aura::client::FocusChangeObserver:
  virtual void OnWindowFocused(aura::Window* gained_focus,
                               aura::Window* lost_focus) OVERRIDE {
    if (window_ && lost_focus == window_) {
      delete lost_focus;
      did_delete_ = true;
    }
  }

  // Overridden from WindowDeleter:
  virtual aura::Window* GetDeletedWindow() OVERRIDE {
    return did_delete_ ? window_ : NULL;
  }

 private:
  aura::Window* root_;
  aura::Window* window_;
  bool did_delete_;

  DISALLOW_COPY_AND_ASSIGN(DeleteOnLoseFocusChangeObserver);
};

class ScopedFocusNotificationObserver : public FocusNotificationObserver {
 public:
  ScopedFocusNotificationObserver(aura::Window* root_window)
      : root_window_(root_window) {
    aura::client::GetActivationClient(root_window_)->AddObserver(this);
    aura::client::GetFocusClient(root_window_)->AddObserver(this);
  }
  virtual ~ScopedFocusNotificationObserver() {
    aura::client::GetActivationClient(root_window_)->RemoveObserver(this);
    aura::client::GetFocusClient(root_window_)->RemoveObserver(this);
  }

 private:
  aura::Window* root_window_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFocusNotificationObserver);
};

class ScopedTargetFocusNotificationObserver : public FocusNotificationObserver {
 public:
  ScopedTargetFocusNotificationObserver(aura::Window* root_window, int id)
      : target_(root_window->GetChildById(id)) {
    aura::client::SetActivationChangeObserver(target_, this);
    aura::client::SetFocusChangeObserver(target_, this);
    tracker_.Add(target_);
  }
  virtual ~ScopedTargetFocusNotificationObserver() {
    if (tracker_.Contains(target_)) {
      aura::client::SetActivationChangeObserver(target_, NULL);
      aura::client::SetFocusChangeObserver(target_, NULL);
    }
  }

 private:
  aura::Window* target_;
  aura::WindowTracker tracker_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTargetFocusNotificationObserver);
};

class ScopedFocusedTextInputClientChanger
    : public ScopedFocusNotificationObserver {
 public:
  ScopedFocusedTextInputClientChanger(aura::Window* root_window,
                                      ui::TextInputClient* text_input_client)
      : ScopedFocusNotificationObserver(root_window),
        text_input_client_(text_input_client) {}

 private:
  // Overridden from aura::client::FocusChangeObserver:
  virtual void OnWindowFocused(aura::Window* gained_focus,
                               aura::Window* lost_focus) OVERRIDE {
    ui::TextInputFocusManager::GetInstance()->FocusTextInputClient(
        text_input_client_);
  }

  ui::TextInputClient* text_input_client_;
};

// Used to fake the handling of events in the pre-target phase.
class SimpleEventHandler : public ui::EventHandler {
 public:
  SimpleEventHandler() {}
  virtual ~SimpleEventHandler() {}

  // Overridden from ui::EventHandler:
  virtual void OnMouseEvent(ui::MouseEvent* event) OVERRIDE {
    event->SetHandled();
  }
  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE {
    event->SetHandled();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleEventHandler);
};

class FocusShiftingActivationObserver
    : public aura::client::ActivationChangeObserver {
 public:
  explicit FocusShiftingActivationObserver(aura::Window* activated_window)
      : activated_window_(activated_window),
        shift_focus_to_(NULL) {}
  virtual ~FocusShiftingActivationObserver() {}

  void set_shift_focus_to(aura::Window* shift_focus_to) {
    shift_focus_to_ = shift_focus_to;
  }

 private:
  // Overridden from aura::client::ActivationChangeObserver:
  virtual void OnWindowActivated(aura::Window* gained_active,
                                 aura::Window* lost_active) OVERRIDE {
    // Shift focus to a child. This should prevent the default focusing from
    // occurring in FocusController::FocusWindow().
    if (gained_active == activated_window_) {
      aura::client::FocusClient* client =
          aura::client::GetFocusClient(gained_active);
      client->FocusWindow(shift_focus_to_);
    }
  }

  aura::Window* activated_window_;
  aura::Window* shift_focus_to_;

  DISALLOW_COPY_AND_ASSIGN(FocusShiftingActivationObserver);
};

// BaseFocusRules subclass that allows basic overrides of focus/activation to
// be tested. This is intended more as a test that the override system works at
// all, rather than as an exhaustive set of use cases, those should be covered
// in tests for those FocusRules implementations.
class TestFocusRules : public BaseFocusRules {
 public:
  TestFocusRules() : focus_restriction_(NULL) {}

  // Restricts focus and activation to this window and its child hierarchy.
  void set_focus_restriction(aura::Window* focus_restriction) {
    focus_restriction_ = focus_restriction;
  }

  // Overridden from BaseFocusRules:
  virtual bool SupportsChildActivation(aura::Window* window) const OVERRIDE {
    // In FocusControllerTests, only the RootWindow has activatable children.
    return window->GetRootWindow() == window;
  }
  virtual bool CanActivateWindow(aura::Window* window) const OVERRIDE {
    // Restricting focus to a non-activatable child window means the activatable
    // parent outside the focus restriction is activatable.
    bool can_activate =
        CanFocusOrActivate(window) || window->Contains(focus_restriction_);
    return can_activate ? BaseFocusRules::CanActivateWindow(window) : false;
  }
  virtual bool CanFocusWindow(aura::Window* window) const OVERRIDE {
    return CanFocusOrActivate(window) ?
        BaseFocusRules::CanFocusWindow(window) : false;
  }
  virtual aura::Window* GetActivatableWindow(
      aura::Window* window) const OVERRIDE {
    return BaseFocusRules::GetActivatableWindow(
        CanFocusOrActivate(window) ? window : focus_restriction_);
  }
  virtual aura::Window* GetFocusableWindow(
      aura::Window* window) const OVERRIDE {
    return BaseFocusRules::GetFocusableWindow(
        CanFocusOrActivate(window) ? window : focus_restriction_);
  }
  virtual aura::Window* GetNextActivatableWindow(
      aura::Window* ignore) const OVERRIDE {
    aura::Window* next_activatable =
        BaseFocusRules::GetNextActivatableWindow(ignore);
    return CanFocusOrActivate(next_activatable) ?
        next_activatable : GetActivatableWindow(focus_restriction_);
  }

 private:
  bool CanFocusOrActivate(aura::Window* window) const {
    return !focus_restriction_ || focus_restriction_->Contains(window);
  }

  aura::Window* focus_restriction_;

  DISALLOW_COPY_AND_ASSIGN(TestFocusRules);
};

// Common infrastructure shared by all FocusController test types.
class FocusControllerTestBase : public aura::test::AuraTestBase {
 protected:
  FocusControllerTestBase() {}

  // Overridden from aura::test::AuraTestBase:
  virtual void SetUp() OVERRIDE {
    wm_state_.reset(new wm::WMState);
    // FocusController registers itself as an Env observer so it can catch all
    // window initializations, including the root_window()'s, so we create it
    // before allowing the base setup.
    test_focus_rules_ = new TestFocusRules;
    focus_controller_.reset(new FocusController(test_focus_rules_));
    aura::test::AuraTestBase::SetUp();
    root_window()->AddPreTargetHandler(focus_controller_.get());
    aura::client::SetFocusClient(root_window(), focus_controller_.get());
    aura::client::SetActivationClient(root_window(), focus_controller_.get());

    // Hierarchy used by all tests:
    // root_window
    //       +-- w1
    //       |    +-- w11
    //       |    +-- w12
    //       +-- w2
    //       |    +-- w21
    //       |         +-- w211
    //       +-- w3
    aura::Window* w1 = aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 1,
        gfx::Rect(0, 0, 50, 50), root_window());
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 11,
        gfx::Rect(5, 5, 10, 10), w1);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 12,
        gfx::Rect(15, 15, 10, 10), w1);
    aura::Window* w2 = aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 2,
        gfx::Rect(75, 75, 50, 50), root_window());
    aura::Window* w21 = aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 21,
        gfx::Rect(5, 5, 10, 10), w2);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 211,
        gfx::Rect(1, 1, 5, 5), w21);
    aura::test::CreateTestWindowWithDelegate(
        aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate(), 3,
        gfx::Rect(125, 125, 50, 50), root_window());
  }
  virtual void TearDown() OVERRIDE {
    root_window()->RemovePreTargetHandler(focus_controller_.get());
    aura::test::AuraTestBase::TearDown();
    test_focus_rules_ = NULL;  // Owned by FocusController.
    focus_controller_.reset();
    wm_state_.reset();
  }

  void FocusWindow(aura::Window* window) {
    aura::client::GetFocusClient(root_window())->FocusWindow(window);
  }
  aura::Window* GetFocusedWindow() {
    return aura::client::GetFocusClient(root_window())->GetFocusedWindow();
  }
  int GetFocusedWindowId() {
    aura::Window* focused_window = GetFocusedWindow();
    return focused_window ? focused_window->id() : -1;
  }
  void ActivateWindow(aura::Window* window) {
    aura::client::GetActivationClient(root_window())->ActivateWindow(window);
  }
  void DeactivateWindow(aura::Window* window) {
    aura::client::GetActivationClient(root_window())->DeactivateWindow(window);
  }
  aura::Window* GetActiveWindow() {
    return aura::client::GetActivationClient(root_window())->GetActiveWindow();
  }
  int GetActiveWindowId() {
    aura::Window* active_window = GetActiveWindow();
    return active_window ? active_window->id() : -1;
  }

  TestFocusRules* test_focus_rules() { return test_focus_rules_; }

  // Test functions.
  virtual void BasicFocus() = 0;
  virtual void BasicActivation() = 0;
  virtual void FocusEvents() = 0;
  virtual void DuplicateFocusEvents() {}
  virtual void ActivationEvents() = 0;
  virtual void ReactivationEvents() {}
  virtual void DuplicateActivationEvents() {}
  virtual void ShiftFocusWithinActiveWindow() {}
  virtual void ShiftFocusToChildOfInactiveWindow() {}
  virtual void ShiftFocusToParentOfFocusedWindow() {}
  virtual void FocusRulesOverride() = 0;
  virtual void ActivationRulesOverride() = 0;
  virtual void ShiftFocusOnActivation() {}
  virtual void ShiftFocusOnActivationDueToHide() {}
  virtual void NoShiftActiveOnActivation() {}
  virtual void NoFocusChangeOnClickOnCaptureWindow() {}
  virtual void ChangeFocusWhenNothingFocusedAndCaptured() {}
  virtual void DontPassDeletedWindow() {}
  virtual void FocusedTextInputClient() {}

 private:
  scoped_ptr<FocusController> focus_controller_;
  TestFocusRules* test_focus_rules_;
  scoped_ptr<wm::WMState> wm_state_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerTestBase);
};

// Test base for tests where focus is directly set to a target window.
class FocusControllerDirectTestBase : public FocusControllerTestBase {
 protected:
  FocusControllerDirectTestBase() {}

  // Different test types shift focus in different ways.
  virtual void FocusWindowDirect(aura::Window* window) = 0;
  virtual void ActivateWindowDirect(aura::Window* window) = 0;
  virtual void DeactivateWindowDirect(aura::Window* window) = 0;

  // Input events do not change focus if the window can not be focused.
  virtual bool IsInputEvent() = 0;

  void FocusWindowById(int id) {
    aura::Window* window = root_window()->GetChildById(id);
    DCHECK(window);
    FocusWindowDirect(window);
  }
  void ActivateWindowById(int id) {
    aura::Window* window = root_window()->GetChildById(id);
    DCHECK(window);
    ActivateWindowDirect(window);
  }

  // Overridden from FocusControllerTestBase:
  virtual void BasicFocus() OVERRIDE {
    EXPECT_EQ(NULL, GetFocusedWindow());
    FocusWindowById(1);
    EXPECT_EQ(1, GetFocusedWindowId());
    FocusWindowById(2);
    EXPECT_EQ(2, GetFocusedWindowId());
  }
  virtual void BasicActivation() OVERRIDE {
    EXPECT_EQ(NULL, GetActiveWindow());
    ActivateWindowById(1);
    EXPECT_EQ(1, GetActiveWindowId());
    ActivateWindowById(2);
    EXPECT_EQ(2, GetActiveWindowId());
    // Verify that attempting to deactivate NULL does not crash and does not
    // change activation.
    DeactivateWindow(NULL);
    EXPECT_EQ(2, GetActiveWindowId());
    DeactivateWindow(GetActiveWindow());
    EXPECT_EQ(1, GetActiveWindowId());
  }
  virtual void FocusEvents() OVERRIDE {
    ScopedFocusNotificationObserver root_observer(root_window());
    ScopedTargetFocusNotificationObserver observer1(root_window(), 1);
    ScopedTargetFocusNotificationObserver observer2(root_window(), 2);

    root_observer.ExpectCounts(0, 0);
    observer1.ExpectCounts(0, 0);
    observer2.ExpectCounts(0, 0);

    FocusWindowById(1);
    root_observer.ExpectCounts(1, 1);
    observer1.ExpectCounts(1, 1);
    observer2.ExpectCounts(0, 0);

    FocusWindowById(2);
    root_observer.ExpectCounts(2, 2);
    observer1.ExpectCounts(2, 2);
    observer2.ExpectCounts(1, 1);
  }
  virtual void DuplicateFocusEvents() OVERRIDE {
    // Focusing an existing focused window should not resend focus events.
    ScopedFocusNotificationObserver root_observer(root_window());
    ScopedTargetFocusNotificationObserver observer1(root_window(), 1);

    root_observer.ExpectCounts(0, 0);
    observer1.ExpectCounts(0, 0);

    FocusWindowById(1);
    root_observer.ExpectCounts(1, 1);
    observer1.ExpectCounts(1, 1);

    FocusWindowById(1);
    root_observer.ExpectCounts(1, 1);
    observer1.ExpectCounts(1, 1);
  }
  virtual void ActivationEvents() OVERRIDE {
    ActivateWindowById(1);

    ScopedFocusNotificationObserver root_observer(root_window());
    ScopedTargetFocusNotificationObserver observer1(root_window(), 1);
    ScopedTargetFocusNotificationObserver observer2(root_window(), 2);

    root_observer.ExpectCounts(0, 0);
    observer1.ExpectCounts(0, 0);
    observer2.ExpectCounts(0, 0);

    ActivateWindowById(2);
    root_observer.ExpectCounts(1, 1);
    observer1.ExpectCounts(1, 1);
    observer2.ExpectCounts(1, 1);
  }
  virtual void ReactivationEvents() OVERRIDE {
    ActivateWindowById(1);
    ScopedFocusNotificationObserver root_observer(root_window());
    EXPECT_EQ(0, root_observer.reactivation_count());
    root_window()->GetChildById(2)->Hide();
    // When we attempt to activate "2", which cannot be activated because it
    // is not visible, "1" will be reactivated.
    ActivateWindowById(2);
    EXPECT_EQ(1, root_observer.reactivation_count());
    EXPECT_EQ(root_window()->GetChildById(2),
              root_observer.reactivation_requested_window());
    EXPECT_EQ(root_window()->GetChildById(1),
              root_observer.reactivation_actual_window());
  }
  virtual void DuplicateActivationEvents() OVERRIDE {
    // Activating an existing active window should not resend activation events.
    ActivateWindowById(1);

    ScopedFocusNotificationObserver root_observer(root_window());
    ScopedTargetFocusNotificationObserver observer1(root_window(), 1);
    ScopedTargetFocusNotificationObserver observer2(root_window(), 2);

    root_observer.ExpectCounts(0, 0);
    observer1.ExpectCounts(0, 0);
    observer2.ExpectCounts(0, 0);

    ActivateWindowById(2);
    root_observer.ExpectCounts(1, 1);
    observer1.ExpectCounts(1, 1);
    observer2.ExpectCounts(1, 1);

    ActivateWindowById(2);
    root_observer.ExpectCounts(1, 1);
    observer1.ExpectCounts(1, 1);
    observer2.ExpectCounts(1, 1);
  }
  virtual void ShiftFocusWithinActiveWindow() OVERRIDE {
    ActivateWindowById(1);
    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());
    FocusWindowById(11);
    EXPECT_EQ(11, GetFocusedWindowId());
    FocusWindowById(12);
    EXPECT_EQ(12, GetFocusedWindowId());
  }
  virtual void ShiftFocusToChildOfInactiveWindow() OVERRIDE {
    ActivateWindowById(2);
    EXPECT_EQ(2, GetActiveWindowId());
    EXPECT_EQ(2, GetFocusedWindowId());
    FocusWindowById(11);
    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(11, GetFocusedWindowId());
  }
  virtual void ShiftFocusToParentOfFocusedWindow() OVERRIDE {
    ActivateWindowById(1);
    EXPECT_EQ(1, GetFocusedWindowId());
    FocusWindowById(11);
    EXPECT_EQ(11, GetFocusedWindowId());
    FocusWindowById(1);
    // Focus should _not_ shift to the parent of the already-focused window.
    EXPECT_EQ(11, GetFocusedWindowId());
  }
  virtual void FocusRulesOverride() OVERRIDE {
    EXPECT_EQ(NULL, GetFocusedWindow());
    FocusWindowById(11);
    EXPECT_EQ(11, GetFocusedWindowId());

    test_focus_rules()->set_focus_restriction(root_window()->GetChildById(211));
    FocusWindowById(12);
    // Input events leave focus unchanged; direct API calls will change focus
    // to the restricted window.
    int focused_window = IsInputEvent() ? 11 : 211;
    EXPECT_EQ(focused_window, GetFocusedWindowId());

    test_focus_rules()->set_focus_restriction(NULL);
    FocusWindowById(12);
    EXPECT_EQ(12, GetFocusedWindowId());
  }
  virtual void ActivationRulesOverride() OVERRIDE {
    ActivateWindowById(1);
    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());

    aura::Window* w3 = root_window()->GetChildById(3);
    test_focus_rules()->set_focus_restriction(w3);

    ActivateWindowById(2);
    // Input events leave activation unchanged; direct API calls will activate
    // the restricted window.
    int active_window = IsInputEvent() ? 1 : 3;
    EXPECT_EQ(active_window, GetActiveWindowId());
    EXPECT_EQ(active_window, GetFocusedWindowId());

    test_focus_rules()->set_focus_restriction(NULL);
    ActivateWindowById(2);
    EXPECT_EQ(2, GetActiveWindowId());
    EXPECT_EQ(2, GetFocusedWindowId());
  }
  virtual void ShiftFocusOnActivation() OVERRIDE {
    // When a window is activated, by default that window is also focused.
    // An ActivationChangeObserver may shift focus to another window within the
    // same activatable window.
    ActivateWindowById(2);
    EXPECT_EQ(2, GetFocusedWindowId());
    ActivateWindowById(1);
    EXPECT_EQ(1, GetFocusedWindowId());

    ActivateWindowById(2);

    aura::Window* target = root_window()->GetChildById(1);
    aura::client::ActivationClient* client =
        aura::client::GetActivationClient(root_window());

    scoped_ptr<FocusShiftingActivationObserver> observer(
        new FocusShiftingActivationObserver(target));
    observer->set_shift_focus_to(target->GetChildById(11));
    client->AddObserver(observer.get());

    ActivateWindowById(1);

    // w1's ActivationChangeObserver shifted focus to this child, pre-empting
    // FocusController's default setting.
    EXPECT_EQ(11, GetFocusedWindowId());

    ActivateWindowById(2);
    EXPECT_EQ(2, GetFocusedWindowId());

    // Simulate a focus reset by the ActivationChangeObserver. This should
    // trigger the default setting in FocusController.
    observer->set_shift_focus_to(NULL);
    ActivateWindowById(1);
    EXPECT_EQ(1, GetFocusedWindowId());

    client->RemoveObserver(observer.get());

    ActivateWindowById(2);
    EXPECT_EQ(2, GetFocusedWindowId());
    ActivateWindowById(1);
    EXPECT_EQ(1, GetFocusedWindowId());
  }
  virtual void ShiftFocusOnActivationDueToHide() OVERRIDE {
    // Similar to ShiftFocusOnActivation except the activation change is
    // triggered by hiding the active window.
    ActivateWindowById(1);
    EXPECT_EQ(1, GetFocusedWindowId());

    // Removes window 3 as candidate for next activatable window.
    root_window()->GetChildById(3)->Hide();
    EXPECT_EQ(1, GetFocusedWindowId());

    aura::Window* target = root_window()->GetChildById(2);
    aura::client::ActivationClient* client =
        aura::client::GetActivationClient(root_window());

    scoped_ptr<FocusShiftingActivationObserver> observer(
        new FocusShiftingActivationObserver(target));
    observer->set_shift_focus_to(target->GetChildById(21));
    client->AddObserver(observer.get());

    // Hide the active window.
    root_window()->GetChildById(1)->Hide();

    EXPECT_EQ(21, GetFocusedWindowId());

    client->RemoveObserver(observer.get());
  }
  virtual void NoShiftActiveOnActivation() OVERRIDE {
    // When a window is activated, we need to prevent any change to activation
    // from being made in response to an activation change notification.
  }

  virtual void NoFocusChangeOnClickOnCaptureWindow() OVERRIDE {
    scoped_ptr<aura::client::DefaultCaptureClient> capture_client(
        new aura::client::DefaultCaptureClient(root_window()));
    // Clicking on a window which has capture should not cause a focus change
    // to the window. This test verifies whether that is indeed the case.
    ActivateWindowById(1);

    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());

    aura::Window* w2 = root_window()->GetChildById(2);
    aura::client::GetCaptureClient(root_window())->SetCapture(w2);
    aura::test::EventGenerator generator(root_window(), w2);
    generator.ClickLeftButton();

    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());
    aura::client::GetCaptureClient(root_window())->ReleaseCapture(w2);
  }

  // Verifies focus change is honored while capture held.
  virtual void ChangeFocusWhenNothingFocusedAndCaptured() OVERRIDE {
    scoped_ptr<aura::client::DefaultCaptureClient> capture_client(
        new aura::client::DefaultCaptureClient(root_window()));
    aura::Window* w1 = root_window()->GetChildById(1);
    aura::client::GetCaptureClient(root_window())->SetCapture(w1);

    EXPECT_EQ(-1, GetActiveWindowId());
    EXPECT_EQ(-1, GetFocusedWindowId());

    FocusWindowById(1);

    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());

    aura::client::GetCaptureClient(root_window())->ReleaseCapture(w1);
  }

  // Verifies if a window that loses activation or focus is deleted during
  // observer notification we don't pass the deleted window to other observers.
  virtual void DontPassDeletedWindow() OVERRIDE {
    FocusWindowById(1);

    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());

    {
      aura::Window* to_delete = root_window()->GetChildById(1);
      DeleteOnLoseActivationChangeObserver observer1(to_delete);
      RecordingActivationAndFocusChangeObserver observer2(root_window(),
                                                          &observer1);

      FocusWindowById(2);

      EXPECT_EQ(2, GetActiveWindowId());
      EXPECT_EQ(2, GetFocusedWindowId());

      EXPECT_EQ(to_delete, observer1.GetDeletedWindow());
      EXPECT_FALSE(observer2.was_notified_with_deleted_window());
    }

    {
      aura::Window* to_delete = root_window()->GetChildById(2);
      DeleteOnLoseFocusChangeObserver observer1(to_delete);
      RecordingActivationAndFocusChangeObserver observer2(root_window(),
                                                          &observer1);

      FocusWindowById(3);

      EXPECT_EQ(3, GetActiveWindowId());
      EXPECT_EQ(3, GetFocusedWindowId());

      EXPECT_EQ(to_delete, observer1.GetDeletedWindow());
      EXPECT_FALSE(observer2.was_notified_with_deleted_window());
    }
  }

  // Verifies if the focused text input client is cleared when a window gains
  // or loses the focus.
  virtual void FocusedTextInputClient() OVERRIDE {
    ui::TextInputFocusManager* text_input_focus_manager =
        ui::TextInputFocusManager::GetInstance();
    ui::DummyTextInputClient text_input_client;
    ui::TextInputClient* null_text_input_client = NULL;

    EXPECT_EQ(null_text_input_client,
              text_input_focus_manager->GetFocusedTextInputClient());

    text_input_focus_manager->FocusTextInputClient(&text_input_client);
    EXPECT_EQ(&text_input_client,
              text_input_focus_manager->GetFocusedTextInputClient());
    FocusWindowById(1);
    // The focused text input client gets cleared when a window gets focused
    // unless any of observers sets the focused text input client.
    EXPECT_EQ(null_text_input_client,
              text_input_focus_manager->GetFocusedTextInputClient());

    ScopedFocusedTextInputClientChanger text_input_focus_changer(
        root_window(), &text_input_client);
    EXPECT_EQ(null_text_input_client,
              text_input_focus_manager->GetFocusedTextInputClient());
    FocusWindowById(2);
    // |text_input_focus_changer| sets the focused text input client.
    EXPECT_EQ(&text_input_client,
              text_input_focus_manager->GetFocusedTextInputClient());

    FocusWindow(NULL);
    // The focused text input client gets cleared when a window loses the focus.
    EXPECT_EQ(null_text_input_client,
              text_input_focus_manager->GetFocusedTextInputClient());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerDirectTestBase);
};

// Focus and Activation changes via aura::client::ActivationClient API.
class FocusControllerApiTest : public FocusControllerDirectTestBase {
 public:
  FocusControllerApiTest() {}

 private:
  // Overridden from FocusControllerTestBase:
  virtual void FocusWindowDirect(aura::Window* window) OVERRIDE {
    FocusWindow(window);
  }
  virtual void ActivateWindowDirect(aura::Window* window) OVERRIDE {
    ActivateWindow(window);
  }
  virtual void DeactivateWindowDirect(aura::Window* window) OVERRIDE {
    DeactivateWindow(window);
  }
  virtual bool IsInputEvent() OVERRIDE { return false; }

  DISALLOW_COPY_AND_ASSIGN(FocusControllerApiTest);
};

// Focus and Activation changes via input events.
class FocusControllerMouseEventTest : public FocusControllerDirectTestBase {
 public:
  FocusControllerMouseEventTest() {}

  // Tests that a handled mouse or gesture event does not trigger a window
  // activation.
  void IgnoreHandledEvent() {
    EXPECT_EQ(NULL, GetActiveWindow());
    aura::Window* w1 = root_window()->GetChildById(1);
    SimpleEventHandler handler;
    root_window()->PrependPreTargetHandler(&handler);
    aura::test::EventGenerator generator(root_window(), w1);
    generator.ClickLeftButton();
    EXPECT_EQ(NULL, GetActiveWindow());
    generator.GestureTapAt(w1->bounds().CenterPoint());
    EXPECT_EQ(NULL, GetActiveWindow());
    root_window()->RemovePreTargetHandler(&handler);
    generator.ClickLeftButton();
    EXPECT_EQ(1, GetActiveWindowId());
  }

 private:
  // Overridden from FocusControllerTestBase:
  virtual void FocusWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.ClickLeftButton();
  }
  virtual void ActivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.ClickLeftButton();
  }
  virtual void DeactivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::Window* next_activatable =
        test_focus_rules()->GetNextActivatableWindow(window);
    aura::test::EventGenerator generator(root_window(), next_activatable);
    generator.ClickLeftButton();
  }
  virtual bool IsInputEvent() OVERRIDE { return true; }

  DISALLOW_COPY_AND_ASSIGN(FocusControllerMouseEventTest);
};

class FocusControllerGestureEventTest : public FocusControllerDirectTestBase {
 public:
  FocusControllerGestureEventTest() {}

 private:
  // Overridden from FocusControllerTestBase:
  virtual void FocusWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.GestureTapAt(window->bounds().CenterPoint());
  }
  virtual void ActivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::test::EventGenerator generator(root_window(), window);
    generator.GestureTapAt(window->bounds().CenterPoint());
  }
  virtual void DeactivateWindowDirect(aura::Window* window) OVERRIDE {
    aura::Window* next_activatable =
        test_focus_rules()->GetNextActivatableWindow(window);
    aura::test::EventGenerator generator(root_window(), next_activatable);
    generator.GestureTapAt(window->bounds().CenterPoint());
  }
  virtual bool IsInputEvent() OVERRIDE { return true; }

  DISALLOW_COPY_AND_ASSIGN(FocusControllerGestureEventTest);
};

// Test base for tests where focus is implicitly set to a window as the result
// of a disposition change to the focused window or the hierarchy that contains
// it.
class FocusControllerImplicitTestBase : public FocusControllerTestBase {
 protected:
  explicit FocusControllerImplicitTestBase(bool parent) : parent_(parent) {}

  aura::Window* GetDispositionWindow(aura::Window* window) {
    return parent_ ? window->parent() : window;
  }

  // Change the disposition of |window| in such a way as it will lose focus.
  virtual void ChangeWindowDisposition(aura::Window* window) = 0;

  // Allow each disposition change test to add additional post-disposition
  // change expectations.
  virtual void PostDispostionChangeExpectations() {}

  // Overridden from FocusControllerTestBase:
  virtual void BasicFocus() OVERRIDE {
    EXPECT_EQ(NULL, GetFocusedWindow());

    aura::Window* w211 = root_window()->GetChildById(211);
    FocusWindow(w211);
    EXPECT_EQ(211, GetFocusedWindowId());

    ChangeWindowDisposition(w211);
    // BasicFocusRules passes focus to the parent.
    EXPECT_EQ(parent_ ? 2 : 21, GetFocusedWindowId());
  }
  virtual void BasicActivation() OVERRIDE {
    DCHECK(!parent_) << "Activation tests don't support parent changes.";

    EXPECT_EQ(NULL, GetActiveWindow());

    aura::Window* w2 = root_window()->GetChildById(2);
    ActivateWindow(w2);
    EXPECT_EQ(2, GetActiveWindowId());

    ChangeWindowDisposition(w2);
    EXPECT_EQ(3, GetActiveWindowId());
    PostDispostionChangeExpectations();
  }
  virtual void FocusEvents() OVERRIDE {
    aura::Window* w211 = root_window()->GetChildById(211);
    FocusWindow(w211);

    ScopedFocusNotificationObserver root_observer(root_window());
    ScopedTargetFocusNotificationObserver observer211(root_window(), 211);
    root_observer.ExpectCounts(0, 0);
    observer211.ExpectCounts(0, 0);

    ChangeWindowDisposition(w211);
    root_observer.ExpectCounts(0, 1);
    observer211.ExpectCounts(0, 1);
  }
  virtual void ActivationEvents() OVERRIDE {
    DCHECK(!parent_) << "Activation tests don't support parent changes.";

    aura::Window* w2 = root_window()->GetChildById(2);
    ActivateWindow(w2);

    ScopedFocusNotificationObserver root_observer(root_window());
    ScopedTargetFocusNotificationObserver observer2(root_window(), 2);
    ScopedTargetFocusNotificationObserver observer3(root_window(), 3);
    root_observer.ExpectCounts(0, 0);
    observer2.ExpectCounts(0, 0);
    observer3.ExpectCounts(0, 0);

    ChangeWindowDisposition(w2);
    root_observer.ExpectCounts(1, 1);
    observer2.ExpectCounts(1, 1);
    observer3.ExpectCounts(1, 1);
  }
  virtual void FocusRulesOverride() OVERRIDE {
    EXPECT_EQ(NULL, GetFocusedWindow());
    aura::Window* w211 = root_window()->GetChildById(211);
    FocusWindow(w211);
    EXPECT_EQ(211, GetFocusedWindowId());

    test_focus_rules()->set_focus_restriction(root_window()->GetChildById(11));
    ChangeWindowDisposition(w211);
    // Normally, focus would shift to the parent (w21) but the override shifts
    // it to 11.
    EXPECT_EQ(11, GetFocusedWindowId());

    test_focus_rules()->set_focus_restriction(NULL);
  }
  virtual void ActivationRulesOverride() OVERRIDE {
    DCHECK(!parent_) << "Activation tests don't support parent changes.";

    aura::Window* w1 = root_window()->GetChildById(1);
    ActivateWindow(w1);

    EXPECT_EQ(1, GetActiveWindowId());
    EXPECT_EQ(1, GetFocusedWindowId());

    aura::Window* w3 = root_window()->GetChildById(3);
    test_focus_rules()->set_focus_restriction(w3);

    // Normally, activation/focus would move to w2, but since we have a focus
    // restriction, it should move to w3 instead.
    ChangeWindowDisposition(w1);
    EXPECT_EQ(3, GetActiveWindowId());
    EXPECT_EQ(3, GetFocusedWindowId());

    test_focus_rules()->set_focus_restriction(NULL);
    ActivateWindow(root_window()->GetChildById(2));
    EXPECT_EQ(2, GetActiveWindowId());
    EXPECT_EQ(2, GetFocusedWindowId());
  }

 private:
  // When true, the disposition change occurs to the parent of the window
  // instead of to the window. This verifies that changes occurring in the
  // hierarchy that contains the window affect the window's focus.
  bool parent_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerImplicitTestBase);
};

// Focus and Activation changes in response to window visibility changes.
class FocusControllerHideTest : public FocusControllerImplicitTestBase {
 public:
  FocusControllerHideTest() : FocusControllerImplicitTestBase(false) {}

 protected:
  FocusControllerHideTest(bool parent)
      : FocusControllerImplicitTestBase(parent) {}

  // Overridden from FocusControllerImplicitTestBase:
  virtual void ChangeWindowDisposition(aura::Window* window) OVERRIDE {
    GetDispositionWindow(window)->Hide();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerHideTest);
};

// Focus and Activation changes in response to window parent visibility
// changes.
class FocusControllerParentHideTest : public FocusControllerHideTest {
 public:
  FocusControllerParentHideTest() : FocusControllerHideTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerParentHideTest);
};

// Focus and Activation changes in response to window destruction.
class FocusControllerDestructionTest : public FocusControllerImplicitTestBase {
 public:
  FocusControllerDestructionTest() : FocusControllerImplicitTestBase(false) {}

 protected:
  FocusControllerDestructionTest(bool parent)
      : FocusControllerImplicitTestBase(parent) {}

  // Overridden from FocusControllerImplicitTestBase:
  virtual void ChangeWindowDisposition(aura::Window* window) OVERRIDE {
    delete GetDispositionWindow(window);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerDestructionTest);
};

// Focus and Activation changes in response to window parent destruction.
class FocusControllerParentDestructionTest
    : public FocusControllerDestructionTest {
 public:
  FocusControllerParentDestructionTest()
      : FocusControllerDestructionTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerParentDestructionTest);
};

// Focus and Activation changes in response to window removal.
class FocusControllerRemovalTest : public FocusControllerImplicitTestBase {
 public:
  FocusControllerRemovalTest() : FocusControllerImplicitTestBase(false) {}

 protected:
  FocusControllerRemovalTest(bool parent)
      : FocusControllerImplicitTestBase(parent) {}

  // Overridden from FocusControllerImplicitTestBase:
  virtual void ChangeWindowDisposition(aura::Window* window) OVERRIDE {
    aura::Window* disposition_window = GetDispositionWindow(window);
    disposition_window->parent()->RemoveChild(disposition_window);
    window_owner_.reset(disposition_window);
  }
  virtual void TearDown() OVERRIDE {
    window_owner_.reset();
    FocusControllerImplicitTestBase::TearDown();
  }

 private:
  scoped_ptr<aura::Window> window_owner_;

  DISALLOW_COPY_AND_ASSIGN(FocusControllerRemovalTest);
};

// Focus and Activation changes in response to window parent removal.
class FocusControllerParentRemovalTest : public FocusControllerRemovalTest {
 public:
  FocusControllerParentRemovalTest() : FocusControllerRemovalTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FocusControllerParentRemovalTest);
};


#define FOCUS_CONTROLLER_TEST(TESTCLASS, TESTNAME) \
    TEST_F(TESTCLASS, TESTNAME) { TESTNAME(); }

// Runs direct focus change tests (input events and API calls).
#define DIRECT_FOCUS_CHANGE_TESTS(TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerApiTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerMouseEventTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerGestureEventTest, TESTNAME)

// Runs implicit focus change tests for disposition changes to target.
#define IMPLICIT_FOCUS_CHANGE_TARGET_TESTS(TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerHideTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerDestructionTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerRemovalTest, TESTNAME)

// Runs implicit focus change tests for disposition changes to target's parent
// hierarchy.
#define IMPLICIT_FOCUS_CHANGE_PARENT_TESTS(TESTNAME) \
    /* TODO(beng): parent destruction tests are not supported at
       present due to workspace manager issues. \
    FOCUS_CONTROLLER_TEST(FocusControllerParentDestructionTest, TESTNAME) */ \
    FOCUS_CONTROLLER_TEST(FocusControllerParentHideTest, TESTNAME) \
    FOCUS_CONTROLLER_TEST(FocusControllerParentRemovalTest, TESTNAME)

// Runs all implicit focus change tests (changes to the target and target's
// parent hierarchy)
#define IMPLICIT_FOCUS_CHANGE_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_TARGET_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_PARENT_TESTS(TESTNAME)

// Runs all possible focus change tests.
#define ALL_FOCUS_TESTS(TESTNAME) \
    DIRECT_FOCUS_CHANGE_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_TESTS(TESTNAME)

// Runs focus change tests that apply only to the target. For example,
// implicit activation changes caused by window disposition changes do not
// occur when changes to the containing hierarchy happen.
#define TARGET_FOCUS_TESTS(TESTNAME) \
    DIRECT_FOCUS_CHANGE_TESTS(TESTNAME) \
    IMPLICIT_FOCUS_CHANGE_TARGET_TESTS(TESTNAME)

// - Focuses a window, verifies that focus changed.
ALL_FOCUS_TESTS(BasicFocus);

// - Activates a window, verifies that activation changed.
TARGET_FOCUS_TESTS(BasicActivation);

// - Focuses a window, verifies that focus events were dispatched.
ALL_FOCUS_TESTS(FocusEvents);

// - Focuses or activates a window multiple times, verifies that events are only
//   dispatched when focus/activation actually changes.
DIRECT_FOCUS_CHANGE_TESTS(DuplicateFocusEvents);
DIRECT_FOCUS_CHANGE_TESTS(DuplicateActivationEvents);

// - Activates a window, verifies that activation events were dispatched.
TARGET_FOCUS_TESTS(ActivationEvents);

// - Attempts to active a hidden window, verifies that current window is
//   attempted to be reactivated and the appropriate event dispatched.
FOCUS_CONTROLLER_TEST(FocusControllerApiTest, ReactivationEvents);

// - Input events/API calls shift focus between focusable windows within the
//   active window.
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusWithinActiveWindow);

// - Input events/API calls to a child window of an inactive window shifts
//   activation to the activatable parent and focuses the child.
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusToChildOfInactiveWindow);

// - Input events/API calls to focus the parent of the focused window do not
//   shift focus away from the child.
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusToParentOfFocusedWindow);

// - Verifies that FocusRules determine what can be focused.
ALL_FOCUS_TESTS(FocusRulesOverride);

// - Verifies that FocusRules determine what can be activated.
TARGET_FOCUS_TESTS(ActivationRulesOverride);

// - Verifies that attempts to change focus or activation from a focus or
//   activation change observer are ignored.
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusOnActivation);
DIRECT_FOCUS_CHANGE_TESTS(ShiftFocusOnActivationDueToHide);
DIRECT_FOCUS_CHANGE_TESTS(NoShiftActiveOnActivation);

// Clicking on a window which has capture should not result in a focus change.
DIRECT_FOCUS_CHANGE_TESTS(NoFocusChangeOnClickOnCaptureWindow);

FOCUS_CONTROLLER_TEST(FocusControllerApiTest,
                      ChangeFocusWhenNothingFocusedAndCaptured);

// See description above DontPassDeletedWindow() for details.
FOCUS_CONTROLLER_TEST(FocusControllerApiTest, DontPassDeletedWindow);

// - Verifies that the focused text input client is cleard when the window focus
//   changes.
ALL_FOCUS_TESTS(FocusedTextInputClient);

// If a mouse event was handled, it should not activate a window.
FOCUS_CONTROLLER_TEST(FocusControllerMouseEventTest, IgnoreHandledEvent);

}  // namespace wm
