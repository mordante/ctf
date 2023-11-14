add_executable(run-time)
target_sources(run-time PRIVATE run-time.cpp)
target_link_libraries(run-time PRIVATE ctf nanobench)

add_executable(compile-time)
target_sources(compile-time PRIVATE compile-time.cpp)
target_link_libraries(compile-time PRIVATE ctf nanobench)
