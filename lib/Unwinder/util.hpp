#pragma once

#include <functional>

namespace Unwinder {

class LocalGuard : public std::function<void()>
{
  using _S = std::function<void()>;

public:
  static void do_nothing() noexcept {}

public:
  ~LocalGuard() noexcept { operator()(); }

  LocalGuard()
    : _S(&LocalGuard::do_nothing)
  {
  }

  using _S::_S;

  using _S::operator=;
};

template<typename T>
void
zerolize(T& t) noexcept
{
  memset(&t, 0, sizeof(T));
}

}
