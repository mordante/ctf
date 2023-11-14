include(FetchContent)

# https://nanobench.ankerl.com/index.html
FetchContent_Declare(
  nanobench
  GIT_REPOSITORY https://github.com/martinus/nanobench.git
  GIT_TAG v4.3.11
  GIT_SHALLOW TRUE
  SYSTEM)

set(BOOST_UT_ALLOW_CPM_USE OFF)
FetchContent_Declare(
  ut
  GIT_REPOSITORY https://github.com/boost-ext/ut.git
  GIT_TAG v2.0.0
  GIT_SHALLOW TRUE
  SYSTEM)

FetchContent_MakeAvailable(nanobench ut)
