target_link_libraries(nerduel tinyexpr)
add_dependencies(nerduel tinyexpr)

if (WITH_TEST)
  target_link_libraries(test_game_nerduel tinyexpr)
  add_dependencies(test_game_nerduel tinyexpr)
  target_link_libraries(run_game_nerduel tinyexpr)
  add_dependencies(run_game_nerduel tinyexpr)
endif (WITH_TEST)