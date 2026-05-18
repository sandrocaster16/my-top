cmake_minimum_required(VERSION 3.14)
project(app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# cpp-httplib
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.45.0
)
FetchContent_MakeAvailable(httplib)

# nlohmann_json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

include_directories(include)

add_executable(app
    src/main.cpp
    include/Scanner.cpp
)

target_link_libraries(app
    PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    pthread
)
