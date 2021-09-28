cmake_minimum_required(VERSION 3.5)

project(image-framing LANGUAGES CXX)

add_executable(image-framing main.cpp)

target_compile_features(image-framing PRIVATE cxx_std_17)


find_package(Boost 1.75 REQUIRED)

find_package(JPEG REQUIRED)
find_package(exiv2 REQUIRED)

target_link_libraries(image-framing PRIVATE Boost::boost JPEG::JPEG exiv2lib)
