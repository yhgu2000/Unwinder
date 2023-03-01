# library 目标名统一采用大写



#
# Unwinder
#
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS json)

file(GLOB Unwinder_src Unwinder/*)
if(WIN32)
  list(FILTER Unwinder_src EXCLUDE REGEX ".*linux\..*")
else()
  list(FILTER Unwinder_src EXCLUDE REGEX ".*win64\..*")
endif()

add_library(Unwinder STATIC ${Unwinder_src})

target_include_directories(Unwinder
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Unwinder PUBLIC Boost::boost Boost::json)
if(WIN32)
  target_link_libraries(Unwinder PUBLIC DbgHelp.lib)
endif()

install(TARGETS Unwinder EXPORT ${EXPORT_TARGETS})

install(DIRECTORY Unwinder TYPE INCLUDE PATTERN "*.cpp" EXCLUDE)
