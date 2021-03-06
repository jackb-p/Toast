cmake_minimum_required(VERSION 2.8.7)
project(Toast)

###############################################################################
## get source ## ##############################################################
###############################################################################

file(GLOB_RECURSE sources bot/*.cpp bot/*.hpp ../lib/sqlite3/sqlite3.c)

link_directories(../lib/v8/lib)

###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE
add_executable(Toast ${sources})

# add some compiler flags
set (CMAKE_CXX_FLAGS "-std=c++14 -Wall ${CMAKE_CXX_FLAGS}")

###############################################################################
## dependencies ###############################################################
###############################################################################

find_package(Boost COMPONENTS system thread regex REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

target_link_libraries(Toast PUBLIC
  ${Boost_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CURL_LIBRARIES}
  v8
  v8_libplatform
  v8_libbase
  icui18n
  icuuc
  rt
  dl
  pthread
)

include_directories(
  ${OPENSSL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${CURL_INCLUDE_DIR}
  ../lib/websocketpp
  ../lib/sqlite3
  ../lib/v8
)

# don't know if necessary, too scared to remove
add_definitions(-D_WEBSOCKETPP_CPP11_STL_)

set(CMAKE_BUILD_TYPE Release)
