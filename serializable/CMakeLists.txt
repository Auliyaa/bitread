project(serializable)

add_executable(ser-test
  main.cpp
  settings.cpp
)

target_include_directories(ser-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

