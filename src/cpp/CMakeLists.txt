cmake_minimum_required(VERSION 3.10)
project(PhantomTicTacToe)

set(CMAKE_CXX_STANDARD 17)

set(PLAYER0_INFOSET_PATH "${CMAKE_SOURCE_DIR}/../../data/player0-infoset.txt")
set(PLAYER1_INFOSET_PATH "${CMAKE_SOURCE_DIR}/../../data/player1-infoset.txt")
set(CHECKPOINT_FOLDER "${CMAKE_SOURCE_DIR}/../../checkpoints")

# Set CMAKE_MODULE_PATH and CMAKE_PREFIX_PATH to include ghc-filesystem
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../../deps/ghc-filesystem/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/../../deps/ghc-filesystem")

# Add ghc-filesystem, xtl, xtensor, and xtensor-io subdirectories with binary directories
add_subdirectory(${CMAKE_SOURCE_DIR}/../../deps/ghc-filesystem ${CMAKE_BINARY_DIR}/ghc-filesystem)
add_subdirectory(${CMAKE_SOURCE_DIR}/../../deps/xtl ${CMAKE_BINARY_DIR}/xtl)
add_subdirectory(${CMAKE_SOURCE_DIR}/../../deps/xtensor ${CMAKE_BINARY_DIR}/xtensor)
add_subdirectory(${CMAKE_SOURCE_DIR}/../../deps/xtensor-io ${CMAKE_BINARY_DIR}/xtensor-io)

include_directories(${CMAKE_SOURCE_DIR}/../../deps/ghc-filesystem/include)
include_directories(${CMAKE_SOURCE_DIR}/../../deps/xtl/include)
include_directories(${CMAKE_SOURCE_DIR}/../../deps/xtensor/include)
include_directories(${CMAKE_SOURCE_DIR}/../../deps/xtensor-io/include)

if(NOT EXISTS ${PLAYER1_INFOSET_PATH})
    message(STATUS "checkpoints folder not found, creating it")
    file(MAKE_DIRECTORY ${CHECKPOINT_FOLDER})
endif()

if(NOT EXISTS ${PLAYER0_INFOSET_PATH})
    message(STATUS "player0-infoset.txt not found, unzipping pttt-infosets.zip")
    execute_process(
        COMMAND unzip "pttt-infosets.zip"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../../data
    )
endif()

add_executable(pttt main_pttt.cpp)
add_executable(rps main_rps.cpp)
target_link_libraries(pttt xtensor xtensor-io)
target_link_libraries(rps xtensor xtensor-io)
