cmake_minimum_required(VERSION 3.10)

project(PhantomTicTacToe)

set(PLAYER0_INFOSET_PATH "${CMAKE_SOURCE_DIR}/../../data/player0-infoset.txt")
set(PLAYER1_INFOSET_PATH "${CMAKE_SOURCE_DIR}/../../data/player1-infoset.txt")
set(CHECKPOINT_FOLDER "${CMAKE_SOURCE_DIR}/../../checkpoints")

if(NOT EXISTS ${PLAYER1_INFOSET_PATH})
    message(STATUS "checkpoints folder not found, creating it")
    file(MAKE_DIRECTORY ${CHECKPOINT_FOLDER})
endif()

add_definitions(-DPLAYER0_INFOSET_PATH = ${PLAYER0_INFOSET_PATH})
add_definitions(-DPLAYER1_INFOSET_PATH = ${PLAYER1_INFOSET_PATH})
add_definitions(-DCHECKPOINT_FOLDER = ${CHECKPOINT_FOLDER})


if(NOT EXISTS ${PLAYER0_INFOSET_PATH})
    message(STATUS "player0-infoset.txt not found, unzipping pttt-infosets.zip")
    execute_process(
        COMMAND unzip "pttt-infosets.zip"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../../data
    )
endif()

add_executable(pttt main.cpp)
