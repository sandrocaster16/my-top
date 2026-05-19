cmake_minimum_required(VERSION 3.14)
project(app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include vendor)

add_executable(app
    src/main.cpp
    include/Scanner.cpp
)

target_link_libraries(app
    PRIVATE
    pthread
)
