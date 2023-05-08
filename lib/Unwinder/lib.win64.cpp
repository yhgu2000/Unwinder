#include "lib.hpp"
#include "util.hpp"
#include "util.win64.hpp"
#include <filesystem>

#include <Windows.h>

#include <DbgHelp.h>
#include <TlHelp32.h>

namespace Unwinder {

static std::string
jsonify(const std::vector<std::uint8_t>& bytes)
{
  const char kHexDigit[] = "0123456789ABCDEF";

  std::string ret;
  ret.reserve(bytes.size() * 2);
  for (auto&& b : bytes)
    ret.push_back(kHexDigit[b >> 4]), ret.push_back(kHexDigit[b & 0x0F]);

  return ret;
}

static json::object
jsonify(const ADDRESS64& data, HANDLE process)
{
  json::object obj;

  // 这里总会是 AddrModeFlat
  // switch (data.Mode) {
  //   case AddrMode1616:
  //     obj["Mode"] = "1616";
  //     break;
  //
  //   case AddrMode1632:
  //     obj["Mode"] = "1632";
  //     break;
  //
  //   case AddrModeReal:
  //     obj["Mode"] = "Real";
  //     break;
  //
  //   case AddrModeFlat:
  //     obj["Mode"] = "Flat";
  //     break;
  //
  //   default:
  //     obj["Mode"] = "";
  //     break;
  // }

  obj["Offset"] = data.Offset;
  obj["Segment"] = data.Segment;

  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
  PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;
  DWORD64 displacement = 0;
  if (SymFromAddr(process, data.Offset, &displacement, symbol))
    obj["Symbol"] = symbol->Name;
  else
    obj["Symbol"] = "";

  return obj;
}

static json::object
jsonify(const KDHELP64& data)
{
  json::object ret;

  ret["Thread"] = data.Thread;
  ret["ThCallbackStack"] = data.ThCallbackStack;
  ret["NextCallback"] = data.NextCallback;
  ret["FramePointer"] = data.FramePointer;
  ret["KiCallUserMode"] = data.KiCallUserMode;
  ret["KeUserCallbackDispatcher"] = data.KeUserCallbackDispatcher;
  ret["SystemRangeStart"] = data.SystemRangeStart;
  ret["ThCallbackBStore"] = data.ThCallbackBStore;
  ret["KiUserExceptionDispatcher"] = data.KiUserExceptionDispatcher;
  ret["StackBase"] = data.StackBase;
  ret["StackLimit"] = data.StackLimit;

  // data.Reserved[5];

  return ret;
}

static json::object
jsonify(const STACKFRAME64& data, HANDLE process)
{
  json::object obj;

  obj["PC"] = jsonify(data.AddrPC, process);
  obj["Return"] = jsonify(data.AddrReturn, process);
  obj["Frame"] = jsonify(data.AddrFrame, process);
  obj["Stack"] = jsonify(data.AddrStack, process);
  obj["BStore"] = jsonify(data.AddrBStore, process);

  obj["FuncTableEntry"] = std::size_t(data.FuncTableEntry);

  json::array params;
  for (auto&& i : data.Params)
    params.push_back(i);
  obj["Params"] = std::move(params);

  obj["Far"] = data.Far;
  obj["Virtual"] = data.Virtual;

  // data.Reserved[3];

  obj["KdHelp"] = jsonify(data.KdHelp);

  // data.AddrBStore; // Intel Itanium

  return obj;
}

static json::object
unwind_thread(HANDLE process, unsigned int tid)
{
  HandleGuard thread =
    OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, tid);

  CONTEXT context;
  zerolize(context);
  context.ContextFlags = CONTEXT_FULL;
  if (!GetThreadContext(thread, &context))
    throw err::WinErr(GetLastError());
  // context.Rsp 好像是栈底帧的栈指针，而不是栈顶的（奇怪！）

  STACKFRAME64 stackframe;
  zerolize(stackframe);
  stackframe.AddrPC.Mode = AddrModeFlat;
  stackframe.AddrPC.Offset = context.Rip;
  stackframe.AddrFrame.Mode = AddrModeFlat;
  stackframe.AddrFrame.Offset = context.Rbp;
  stackframe.AddrStack.Mode = AddrModeFlat;
  stackframe.AddrStack.Offset = context.Rsp;

  json::array frames;
  while (StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                     process,
                     thread,
                     &stackframe,
                     &context,
                     NULL,
                     SymFunctionTableAccess64,
                     SymGetModuleBase64,
                     NULL))
    frames.push_back(jsonify(stackframe, process));

  MEMORY_BASIC_INFORMATION stackInfo;
  zerolize(stackInfo);
  if (!VirtualQueryEx(process,
                      reinterpret_cast<LPCVOID>(context.Rsp),
                      &stackInfo,
                      sizeof(stackInfo)))
    throw err::WinErr(GetLastError());
  DWORD64 top =
    frames[0].get_object()["Stack"].get_object()["Offset"].as_uint64();
  DWORD64 bottom =
    reinterpret_cast<DWORD64>(stackInfo.BaseAddress) + stackInfo.RegionSize;
  assert(top <= bottom);

  std::vector<std::uint8_t> data;
  data.resize(bottom - top);
  SIZE_T size;
  if (!ReadProcessMemory(process,
                         reinterpret_cast<LPCVOID>(top),
                         data.data(),
                         bottom - top,
                         &size))
    throw err::WinErr(GetLastError());
  data.resize(size);

  json::object obj;
  obj["frames"] = std::move(frames);
  obj["top"] = top;
  obj["bottom"] = bottom;
  obj["data"] = jsonify(data);

  return obj;
}

json::value
unwind_process(unsigned int pid,
               const std::vector<std::string>& pathes) noexcept(false)
{
  // 为了使 SymInitialize 能成功，这里必须要取得 PROCESS_VM_WRITE 和
  // PROCESS_VM_OPERATION 两个权限！
  HandleGuard process = OpenProcess(PROCESS_SUSPEND_RESUME | PROCESS_VM_READ |
                                      PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                                    FALSE,
                                    pid);

  // 暂停进程中所有线程
  if (!DebugActiveProcess(pid))
    throw err::WinErr(GetLastError());
  LocalGuard guardDebugStop = [=]() { DebugActiveProcessStop(pid); };

  // 初始化符号查询服务
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
  std::string searchPathes;
  for (auto&& path : pathes)
    searchPathes += std::filesystem::absolute(path).string() + ";";

  LocalGuard guardSymCleanup;
  if (SymInitialize(process, searchPathes.c_str(), TRUE))
    guardSymCleanup = [&]() { SymCleanup(process); };

  json::object ret;

  HandleGuard snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);

  THREADENTRY32 threadEntry;
  threadEntry.dwSize = sizeof(THREADENTRY32);
  // https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/ns-tlhelp32-threadentry32
  // If you do not initialize dwSize, Thread32First fails.

  if (Thread32First(snapshot, &threadEntry)) {
    do {
      if (threadEntry.th32OwnerProcessID == pid)
        ret[std::to_string(threadEntry.th32ThreadID)] =
          unwind_thread(process, threadEntry.th32ThreadID);
    } while (Thread32Next(snapshot, &threadEntry));
  }

  return ret;
}

}
