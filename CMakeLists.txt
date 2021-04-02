project(wasm-engines)
set(CMAKE_CXX_STANDARD 17)

include_directories(libs/wasmer/include)
include_directories(engines)

file(GLOB WASMER_LIB "libs/wasmer/lib/libwasmer.so")

add_executable(wasm-engines main.cpp engines/wasmer-instance-db.cpp)

target_link_libraries(wasm-engines ${WASMER_LIB})