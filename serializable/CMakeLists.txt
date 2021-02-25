project(serializable)

set(CMAKE_CXX_STANDARD 11)

add_executable(ser-test
  main.cpp
  settings.cpp
)

target_include_directories(ser-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

