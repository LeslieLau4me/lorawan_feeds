cmake_minimum_required(VERSION 3.18)

project(cgi-cpp)

aux_source_directory(. SRC_LIST)
add_executable(cgi-cpp ${SRC_LIST})
target_link_libraries(cgi-cpp ${toml11} ${stdcpp} ${nlohmannjson} ${easyloggingpp})

install(TARGETS cgi-cpp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
