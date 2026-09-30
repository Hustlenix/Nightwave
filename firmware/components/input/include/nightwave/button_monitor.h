#pragma once

namespace nightwave {

class ButtonMonitor {
  public:
    bool start();

  private:
    static void task_entry(void* context);
    void task();
    void* task_handle_{nullptr};
};

}  // namespace nightwave
