#pragma once

#include "err.hpp"

namespace Unwinder {

namespace err {

class WinErr : public Err
{
public:
  unsigned long _msgid;

public:
  WinErr(unsigned long msgid)
    : _msgid(msgid)
  {
  }

  // exception interface
public:
  virtual const char* what() const noexcept override
  {
    return ERR_WHAT(WinErr);
  }

  // Err interface
public:
  virtual std::string info() const noexcept override;
};

}

class HandleGuard
{
  HandleGuard(const HandleGuard&) = delete;
  HandleGuard(HandleGuard&&) = delete;
  HandleGuard& operator=(const HandleGuard&) = delete;
  HandleGuard& operator=(HandleGuard&&) = delete;

public:
  void* _;

public:
  HandleGuard(void* _);

  ~HandleGuard() noexcept;

  operator void*() { return _; }

public:
  template<typename T>
  T cast()
  {
    return static_cast<T>(_);
  }
};

}
