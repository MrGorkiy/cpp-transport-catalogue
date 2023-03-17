cmake_minimum_required(VERSION 3.23)
project(cpp_transport_catalogue)

set(CMAKE_CXX_STANDARD 20)

include_directories(transport-catalogue)

add_executable(cpp_transport_catalogue
        transport-catalogue/geo.h
        transport-catalogue/input_reader.cpp
        transport-catalogue/input_reader.h
        transport-catalogue/main.cpp
        transport-catalogue/stat_reader.cpp
        transport-catalogue/stat_reader.h
        transport-catalogue/transport_catalogue.cpp
        transport-catalogue/transport_catalogue.h)
