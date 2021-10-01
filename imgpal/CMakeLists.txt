find_package(OpenCV COMPONENTS core imgcodecs imgproc photo REQUIRED)
find_package(OpenMP REQUIRED)

add_executable(imgpal imgpal.cpp)

target_link_libraries(imgpal PRIVATE opencv_core opencv_imgcodecs opencv_imgproc opencv_photo OpenMP::OpenMP_CXX)
