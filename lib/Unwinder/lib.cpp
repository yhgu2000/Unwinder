#include "lib.hpp"

#define self (*this)

namespace Unwinder {

std::ostream&
IndentPrint::operator()(const json::value& jv)
{
  switch (jv.kind()) {
    case json::kind::object: {
      _os << "{\n";
      indent();

      auto const& obj = jv.get_object();
      if (!obj.empty()) {
        auto it = obj.begin(), end = obj.end();
        while (true) {
          prefix() << json::serialize(it->key()) << " : ", self(it->value());
          if (++it == end) {
            _os << "\n";
            break;
          }
          _os << ",\n";
        }
      }

      dedent();
      prefix() << "}";
    } break;

    case json::kind::array: {
      _os << "[\n";
      indent();

      auto const& arr = jv.get_array();
      if (!arr.empty()) {
        auto it = arr.begin(), end = arr.end();
        while (true) {
          prefix(), self(*it);
          if (++it == arr.end()) {
            _os << "\n";
            break;
          }
          _os << ",\n";
        }
      }

      dedent();
      prefix() << "]";
    } break;

    case json::kind::string:
      _os << json::serialize(jv.get_string());
      break;

    case json::kind::uint64:
      _os << jv.get_uint64();
      break;

    case json::kind::int64:
      _os << jv.get_int64();
      break;

    case json::kind::double_:
      _os << jv.get_double();
      break;

    case json::kind::bool_:
      if (jv.get_bool())
        prefix() << "true";
      else
        prefix() << "false";
      break;

    case json::kind::null:
      prefix() << "null";
      break;
  }

  return _os;
}

std::ostream&
IndentPrint::prefix()
{
  for (auto i = 0U; i < _indent; ++i)
    _os.put(' ');
  return _os;
}

}
