#pragma once

#include <exception>
#include <string>

namespace Unwinder {

namespace err {
class Lit;
class Str;
}

class Err : public std::exception
{
public:
  Err() {}

public:
  /**
   * @brief 构造并返回人类可读的错误信息字符串
   */
  virtual std::string info() const noexcept { return what(); }

  // exception interface
public:
  virtual const char* what() const noexcept override { return "class ::EFA;"; }

#ifdef _MSC_VER
private:
  friend class err::Lit;
  friend class err::Str;

  Err(const char* msg)
    : std::exception(msg)
  {
  }
#endif
};

namespace err {

#define ERR_WHAT(cls) "class ::EFA::err::" #cls ";"

class Lit : public Err
{
public:
#ifdef _MSC_VER

  Lit(const char* what)
    : Err(what)
  {
  }

  using std::exception::what;

#else
  const char* _what;

  Lit(const char* what)
    : _what(what)
  {
  }

  // exception interface
public:
  virtual const char* what() const noexcept override { return _what.c_str(); }
#endif

  // Err interface
public:
  virtual std::string info() const noexcept override { return what(); }
};

class Str : public Err
{
public:
  std::string _what;

public:
#ifdef _MSC_VER
  Str(std::string what)
    : Err(what.c_str())
    , _what(std::move(what))
  {
  }

#else
  Str(std::string what)
    : _what(std::move(what))
  {
  }

  // exception interface
public:
  virtual const char* what() const noexcept override { return _what.c_str(); }
#endif

  // Err interface
public:
  virtual std::string info() const noexcept override { return _what; }
};

class Errno : public Err
{
public:
  int _errnum;

public:
  Errno(int errnum)
    : _errnum(errnum)
  {
  }

  // exception interface
public:
  virtual const char* what() const noexcept override
  {
    return strerror(_errnum);
  }

  // Err interface
public:
  virtual std::string info() const noexcept override
  {
    return strerror(_errnum);
  }
};

}

}
