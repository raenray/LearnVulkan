#pragma once
#include <chrono>

namespace Engine
{
namespace Platform
{
class Timer
{
public:
    using Clock = std::chrono::high_resolution_clock;

    Timer()
    {
        start_ = Clock::now();
        last_ = start_;
    }

    void tick()
    {
        auto now = Clock::now();
        deltaTime_ = firstTick_ ? std::chrono::duration<float>(now - start_).count()
                                : std::chrono::duration<float>(now - last_).count();
        elapsedTime_ = std::chrono::duration<float>(now - start_).count();
        last_ = now;
        firstTick_ = false;
    }

    float deltaTime() const { return deltaTime_; }

    float elapsedTime() const { return elapsedTime_; }

private:
    Clock::time_point start_, last_;
    float deltaTime_ = 0, elapsedTime_ = 0;
    bool firstTick_ = true;
};
} // namespace Platform
} // namespace Engine
