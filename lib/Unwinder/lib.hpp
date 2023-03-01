#pragma once

#include "err.hpp"
#include <boost/json.hpp>

namespace Unwinder {

namespace json = boost::json;

/**
 * @brief 以缩进形式打印 boost::json 值
 */
class IndentPrint
{
public:
  IndentPrint(std::ostream& os)
    : _os(os)
    , _indent(0)
  {
  }

public:
  std::ostream& operator()(const json::value& jv);

private:
  std::ostream& _os;
  unsigned int _indent;

private:
  std::ostream& prefix();
  void indent() { _indent += 2; }
  void dedent() { _indent -= 2; }
};

/**
 * @brief 对进程中的所有线程进行栈展开，将所得信息格式化为 JSON 返回。
 *
 * @param pid 进程号
 * @param pathes 额外的符号查找目录
 * @return JSON 值，其结构取决于平台，具体说明见文档。
 */
json::value
unwind_process(unsigned int pid,
               const std::vector<std::string>& pathes) noexcept(false);

}
