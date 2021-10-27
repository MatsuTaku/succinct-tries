cmake_minimum_required(VERSION 3.17)

project(succinct-tries)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(include_dir ${CMAKE_SOURCE_DIR}/include)
set(link_dir ${CMAKE_SOURCE_DIR}/lib)

# Install SDSL-lite
if(NOT EXISTS ${include_dir}/sdsl)
    set(sdsl_dir "${CMAKE_CURRENT_SOURCE_DIR}/sdsl-lite-2.1.1")
    if(NOT EXISTS ${sdsl_dir})
        execute_process(COMMAND tar -xf ${sdsl_dir}.tar.gz.offline.install.gz
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    execute_process(COMMAND ${sdsl_dir}/install.sh ${CMAKE_SOURCE_DIR})
endif()

include_directories(/usr/local/include ${include_dir})
link_libraries(${link_dir}/libsdsl.a)

add_executable(example example.cpp)
