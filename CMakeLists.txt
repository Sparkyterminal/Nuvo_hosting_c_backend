cmake_minimum_required(VERSION 3.10)
project(location_tracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(location_tracker
    main.cpp
    locationTracker.cpp
)

target_include_directories(location_tracker PRIVATE . boost)
target_link_libraries(location_tracker PRIVATE ws2_32 mswsock)
