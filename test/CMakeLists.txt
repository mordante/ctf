add_executable(unittest)
target_sources(unittest PRIVATE format.cpp main.cpp string_view.cpp valid.cpp)
target_link_libraries(unittest PRIVATE ctf ut)

# Uses Clang's verify to validate the expected compiler diagnostics.
#
# Since it makes no sense to test this when the source files are "broken" the
# command depends on the unit tests. (It assumes when they build the library is
# correct.)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/diagnostics.cpp.stamp"
  DEPENDS unittest "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics.cpp"
  COMMAND
    ${CMAKE_CXX_COMPILER} -stdlib=libc++ -std=c++26 -fsyntax-only
    -ferror-limit=0 -I "${CMAKE_SOURCE_DIR}/include" -Xclang -verify
    "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics.cpp"
  COMMAND ${CMAKE_COMMAND} -E touch
          "${CMAKE_CURRENT_BINARY_DIR}/diagnostics.cpp.stamp"
  COMMENT "Verify the compiler diagnostics")

add_custom_target(verify ALL
                  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/diagnostics.cpp.stamp")
