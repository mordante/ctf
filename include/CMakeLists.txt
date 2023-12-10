add_library(ctf INTERFACE)
target_sources(
  ctf INTERFACE ctf/format.hpp ctf/format_error.hpp ctf/formatter.hpp
                ctf/parse.hpp ctf/tuple.hpp ctf/utility.hpp)
target_include_directories(ctf INTERFACE .)
