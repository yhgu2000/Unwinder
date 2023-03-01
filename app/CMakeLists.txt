# app 可执行目标名统一采用小写，如出现和库目标重名的情况，建议在之后添加 -app 后缀，
# 然后设置目标的 OUTPUT_NAME 修正生成的文件名。



#
# unwind2json
#
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(unwind2json unwind2json.cpp)

target_link_libraries(unwind2json PUBLIC Unwinder
  Boost::boost Boost::program_options
)

install(TARGETS unwind2json EXPORT ${EXPORT_TARGETS})



#
# echotest
#
add_executable(echotest echotest.cpp)
