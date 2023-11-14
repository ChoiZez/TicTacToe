add_library(userData "")

target_sources(userData
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/userData.h
)

target_include_directories(userData
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(userData PROPERTIES LINKER_LANGUAGE CXX)