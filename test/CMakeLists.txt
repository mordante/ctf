add_executable(unittest)
target_sources(unittest PRIVATE format.cpp main.cpp)
target_link_libraries(unittest PRIVATE ctf ut)
