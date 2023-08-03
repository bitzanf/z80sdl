//
// Created by filip on 29.7.23.
//

//https://stackoverflow.com/questions/30425772/c-11-calling-a-c-function-periodically

#ifndef Z80SDL_CALLBACKTIMER_HPP
#define Z80SDL_CALLBACKTIMER_HPP

#include <thread>
#include <functional>

class CallBackTimer {
public:
    CallBackTimer();
    ~CallBackTimer();

    void stop();
    void start(int interval_ms, std::function<void(void)> func);
    [[nodiscard]] bool is_running() const noexcept;

private:
    std::atomic<bool> isRunning;
    std::thread timerThread;

    void maybeStop();
};


#endif //Z80SDL_CALLBACKTIMER_HPP
