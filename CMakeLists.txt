cmake_minimum_required(VERSION 3.23)
project(cpp_transport_catalogue)

set(CMAKE_CXX_STANDARD 20)

add_executable(Sprint11Topic2Lesson14 transport-catalogue/main.cpp
        transport-catalogue/domain.cpp
        transport-catalogue/domain.h
        transport-catalogue/geo.cpp
        transport-catalogue/geo.h
        transport-catalogue/json.cpp
        transport-catalogue/json.h
        transport-catalogue/json_reader.cpp
        transport-catalogue/json_reader.h
        transport-catalogue/map_renderer.cpp
        transport-catalogue/map_renderer.h
        transport-catalogue/request_handler.cpp
        transport-catalogue/request_handler.h
        transport-catalogue/svg.cpp
        transport-catalogue/svg.h
        transport-catalogue/transport_catalogue.cpp
        transport-catalogue/transport_catalogue.h)
