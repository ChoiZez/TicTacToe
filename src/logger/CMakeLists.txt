add_library(logger "")

target_sources(logger
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/logger.h
)

target_include_directories(logger
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)