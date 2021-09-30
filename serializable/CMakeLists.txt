add_executable(serializable
  main.cpp
  settings.cpp
)

target_include_directories(serializable PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

