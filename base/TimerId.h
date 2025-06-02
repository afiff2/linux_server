#pragma once

#include "Timer.h"
#include <memory>

class TimerId
{
  public:
    TimerId() = default;
    explicit TimerId(std::shared_ptr<Timer> timer) : timer_(timer) {}

    std::shared_ptr<Timer> timer() const { return timer_.lock(); }

    static TimerId invalid() {
        return TimerId();
    }

  private:
    std::weak_ptr<Timer> timer_;
};