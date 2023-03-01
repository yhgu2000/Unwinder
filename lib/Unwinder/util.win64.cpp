#include "util.win64.hpp"

#include <Windows.h>

#include <DbgHelp.h>
#include <TlHelp32.h>

namespace Unwinder {

namespace err {

std::string
WinErr::info() const noexcept
{
  LPTSTR msgbuf;

  auto len =
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  _msgid,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&msgbuf,
                  0,
                  NULL);
  if (len == 0)
    return "[[ 'FormatMessage' failed: " + std::to_string(GetLastError()) + " ]]";

  std::string msg(msgbuf);

  LocalFree(msgbuf);

  return msg;
}

}

HandleGuard::HandleGuard(void* _)
  : _(_)
{
  if (!_)
    throw err::WinErr(GetLastError());
}

HandleGuard::~HandleGuard() noexcept
{
  CloseHandle(_);
}

}
