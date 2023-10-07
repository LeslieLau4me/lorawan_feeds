cmake_minimum_required(VERSION 3.18)

project(bridge-sub-test)
FIND_LIBRARY(mosquitto NAMES mosquitto)
set(SRC_FILES
    ./bridge-sub-test.cpp
)

add_executable(bridge-sub-test ${SRC_FILES})
target_link_libraries(bridge-sub-test ${stdcpp} ${mosquitto})
target_link_libraries(${PROJECT_NAME} mosquitto)

install(TARGETS bridge-sub-test RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})