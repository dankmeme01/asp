cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asp VERSION 0.2.0)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_library(asp STATIC ${HEADERS} ${SOURCES})

target_include_directories(asp PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(asp PRIVATE include/)

install(TARGETS asp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
