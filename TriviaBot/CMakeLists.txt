cmake_minimum_required(VERSION 2.8)

## section: Macro
MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledBinary}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

## start setting
SET (this_target TriviaBot)
PROJECT(${this_target})



## section: include directory
INCLUDE_DIRECTORIES(
  C:/Users/Jack/Documents/GitHubVisualStudio/TriviaDiscord/lib/boost_1_61_0
	C:/Users/Jack/Documents/GitHubVisualStudio/TriviaDiscord/lib/openssl/include
	C:/Users/Jack/Documents/GitHubVisualStudio/TriviaDiscord/lib/websocketpp
	C:/Users/Jack/Documents/GitHubVisualStudio/TriviaDiscord/lib/cpr/include
	C:/Users/Jack/Documents/GitHubVisualStudio/TriviaDiscord/lib/libcurl/include
  )

## section: source files
# Add your source files here (one file per line), please SORT in alphabetical order for future maintenance
SET (${this_target}_SOURCE_FILES
    ..\lib\cpr\cpr\auth.cpp
	..\lib\cpr\cpr\cookies.cpp
	..\lib\cpr\cpr\cprtypes.cpp
	..\lib\cpr\cpr\digest.cpp
	..\lib\cpr\cpr\error.cpp
	..\lib\cpr\cpr\multipart.cpp
	..\lib\cpr\cpr\parameters.cpp
	..\lib\cpr\cpr\payload.cpp
	..\lib\cpr\cpr\proxies.cpp
	..\lib\cpr\cpr\session.cpp
	..\lib\cpr\cpr\util.cpp
	bot\APIHelper.cpp
	bot\ClientConnection.cpp
	bot\GatewayHandler.cpp
	bot\http\HTTPHelper.cpp
	bot\TriviaBot.cpp
	bot\TriviaGame.cpp
	data_management\LoadDB.cpp
    )

## section: header files
# Add your header files here(one file per line), please SORT in alphabetical order for future maintenance!
SET(${this_target}_HEADER_FILES
    bot\ClientConnection.hpp
	bot\APIHelper.hpp
	bot\data_structures\Channel.hpp
	bot\data_structures\Guild.hpp
	bot\data_structures\User.hpp
	bot\GatewayHandler.hpp
	bot\HTTP\HTTPHelper.hpp
	bot\json\json.hpp
	bot\TriviaGame.hpp
    )

SOURCE_GROUP("Source Files" FILES 
	
	)
SOURCE_GROUP("Header Files" FILES 
	
	)
SOURCE_GROUP("Resource Files" FILES 
	
	)


## section: precompiled header	
#ADD_MSVC_PRECOMPILED_HEADER("precompiled.h" "precompiled.cpp" MySources)
#ADD_LIBRARY(MyLibrary ${MySources})

SET_SOURCE_FILES_PROPERTIES(${this_target}_HEADER_FILES
                            PROPERTIES HEADER_FILE_ONLY TRUE)
LIST(APPEND ${this_target}_SOURCE_FILES ${${this_target}_HEADER_FILES})

## section: add definitions
#   add prefix -D. example> -DSHP
#  - DO NOT add  the following definitions(already defined in ${OSP_DEFINITIONS}:
#   -DSHP, -DWIN32, -D_WINDOWS, -D_DEBUG, -D_USRDLL, -D_CRT_SECURE_NO_DEPRECATE
ADD_DEFINITIONS(
    -D_CRT_SECURE_NO_WARNINGS
    )

## section: add target

ADD_EXECUTABLE(${this_target} ${${this_target}_SOURCE_FILES})

## section: add dependency
# dependency determines overall build order.
ADD_DEPENDENCIES(${this_target} 
    libcurl.lib
	sqlite3.lib
	libeay32.lib
	ssleay32.lib
	sqlite3.lib
	libeay32.lib
	)

## section: set link libraries
TARGET_LINK_LIBRARIES( ${this_target}
        libcurl.lib
	sqlite3.lib
	libeay32.lib
	ssleay32.lib
	sqlite3.lib
	libeay32.lib
        )
