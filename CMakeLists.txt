project(wasm-engines)
set(CMAKE_CXX_STANDARD 17)

include_directories(libs/wasmer/include)
include_directories(libs/v8/include)
include_directories(engines)

find_package(Threads)

file(GLOB WASMER_LIB "libs/wasmer/lib/libwasmer.so")
file(GLOB V8_LIB_MONOLIT "libs/v8/lib/libv8_monolith.a")

add_executable(wasm-engines main.cpp engines/wasmer-instance-db.cpp engines/v8-instance-db.cpp)

target_link_libraries(wasm-engines ${CMAKE_THREAD_LIBS_INIT} ${WASMER_LIB} ${V8_LIB_MONOLIT})

set(CMAKE_BUILD_TYPE "Release")