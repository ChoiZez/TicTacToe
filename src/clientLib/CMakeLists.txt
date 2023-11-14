add_library(clientLib "")

target_sources(clientLib
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/clientSocket.h
)

target_include_directories(clientLib
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(clientLib PROPERTIES LINKER_LANGUAGE CXX)