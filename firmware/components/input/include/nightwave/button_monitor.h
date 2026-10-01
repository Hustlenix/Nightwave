#pragma once
#include "nightwave/input_event.h"

namespace nightwave {

class ButtonMonitor {
  public:
    bool start();
    bool poll(ButtonEvent& event);

  private:
    static void task_entry(void* context);
    void task();
    void* task_handle_{nullptr};
    void* queue_{nullptr};
};

}  // namespace nightwave
