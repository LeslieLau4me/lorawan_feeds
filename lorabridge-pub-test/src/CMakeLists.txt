cmake_minimum_required(VERSION 3.18)

project(bridge-pub-test)
FIND_LIBRARY(mosquitto NAMES mosquitto)
set(SRC_FILES
    ./bridge-pub-test.cpp
)

add_executable(bridge-pub-test ${SRC_FILES})
target_link_libraries(bridge-pub-test ${stdcpp} ${mosquitto} ${toml11})
target_link_libraries(${PROJECT_NAME} mosquitto)

install(TARGETS bridge-pub-test RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})