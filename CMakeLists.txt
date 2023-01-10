cmake_minimum_required(VERSION 3.8)

project(img_viewer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Mosquitto IMPORTED_TARGET libmosquitto REQUIRED)

add_executable(img_viewer src/mqtt_subscription.cpp src/img_viewer.cpp)
target_include_directories(img_viewer PRIVATE third_party/cista/include)
target_link_libraries(img_viewer PRIVATE PkgConfig::Mosquitto)

