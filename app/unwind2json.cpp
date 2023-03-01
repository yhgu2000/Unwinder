#include "project.h"
#include <Unwinder/hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace Unwinder;
using namespace std::string_literals;

namespace fs = std::filesystem;
namespace po = boost::program_options;

int
main(int argc, char* argv[])
try {
  po::options_description od("Options");
  od.add_options()                                   //
    ("version,v", "print version info")              //
    ("help,h", "print help info")                    //
    ("pid", po::value<unsigned int>(), "process id") //
    ("search,s",                                     //
     po::value<std::vector<std::string>>(),          //
     "symbol search path")                           //
    ;

  po::positional_options_description pod;
  pod.add("pid", 1);

  po::variables_map vmap;
  auto parsed = po::command_line_parser(argc, argv)
                  .options(od)
                  .positional(pod)
                  .allow_unregistered()
                  .run();
  po::store(parsed, vmap);
  po::notify(vmap);

  if (vmap.count("version")) {
    std::cout << "unwind2json"
                 "\n"
                 "\nBuilt: " __TIME__ " (" __DATE__ ")"
                 "\nUnwinder: " Unwinder_VERSION "\n"
                 "\nCopyright (C) 2023 Gu Yuhao. All Rights Reserved."
              << std::endl;
    return 0;
  }

  if (vmap.count("help") || argc == 1) {
    std::cout << od << "\n";
    return 0;
  }

  if (vmap.count("pid")) {
    std::vector<std::string> pathes;
    if (vmap.count("search"))
      pathes = std::move(vmap["search"].as<std::vector<std::string>>());

    IndentPrint print(std::cout);
    print(unwind_process(vmap["pid"].as<unsigned int>(), pathes));
    std::cout << std::endl;
    return 0;
  }
}

catch (Err& e) {
  std::cout << "\nERROR! " << e.what() << "\n" << e.info() << std::endl;
  return -3;
}

catch (std::exception& e) {
  std::cout << "\nERROR! " << e.what() << std::endl;
  return -2;
}

catch (...) {
  return -1;
}
