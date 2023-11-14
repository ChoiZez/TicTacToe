add_library(tictactoe "")

target_sources(tictactoe
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/tictactoe.h
        ${CMAKE_CURRENT_LIST_DIR}/gameSession.h
)

target_include_directories(tictactoe
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(tictactoe PROPERTIES LINKER_LANGUAGE CXX)