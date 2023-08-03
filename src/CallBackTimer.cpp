//
// Created by filip on 29.7.23.
//

#include "CallBackTimer.hpp"

CallBackTimer::CallBackTimer() : isRunning(false) {}

CallBackTimer::~CallBackTimer() {
    maybeStop();
}

void CallBackTimer::stop() {
    isRunning.store(false, std::memory_order_release);
    if (timerThread.joinable()) timerThread.join();
}

void CallBackTimer::start(int interval_ms, std::function<void(void)> func) {
    maybeStop();
    isRunning.store(true, std::memory_order_release);
    timerThread = std::thread(
        [this, interval_ms, func]() {
               while (isRunning.load(std::memory_order_acquire)) {
                   func();
                   std::this_thread::sleep_for(
                       std::chrono::milliseconds(interval_ms)
                   );
               }
        }
    );
}

void CallBackTimer::maybeStop() {
    if (isRunning.load(std::memory_order_acquire)) stop();
}

bool CallBackTimer::is_running() const noexcept {
    return isRunning.load(std::memory_order_acquire) && timerThread.joinable();
}
