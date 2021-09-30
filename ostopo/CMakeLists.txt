find_package(Boost COMPONENTS regex system filesystem REQUIRED)

add_executable(ostopo
  src/cgroup.cpp
  src/cpu.cpp
  src/file.cpp
  src/pid.cpp

  ostopo.cpp
)

target_include_directories(ostopo PRIVATE inc)

target_link_libraries(ostopo Boost::regex Boost::system Boost::filesystem)
