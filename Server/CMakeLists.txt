cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}")

project( server )

find_package( OpenCV )

find_package( Boost REQUIRED )
if(Boost_FOUND)
    message("Boost found!")
endif()

LINK_DIRECTORIES( "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.5/lib/Win32")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

set(FFMPEG_ROOT "" CACHE FILEPATH "Root of the FFMPEG directory, which has README.txt")
if (FFMPEG_ROOT)
	FIND_PATH( FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
	  ${FFMPEG_ROOT}/include
	)
	include_directories(${FFMPEG_INCLUDE_DIR})

	FIND_LIBRARY( FFMPEG_LIBRARY_avcodec avcodec
	  ${FFMPEG_ROOT}/lib
	)
	FIND_LIBRARY( FFMPEG_LIBRARY_avutil avutil
	  ${FFMPEG_ROOT}/lib
	)
	if (FFMPEG_LIBRARY_avcodec AND FFMPEG_LIBRARY_avutil)
		set (FFMPEG_FOUND 1)
		message("FFMPEG found!")
	endif()
endif()

#Files for WDDM
find_package( DirectX )
include_directories(${DXGI_INCLUDE})

add_executable( server server.cpp )
target_link_libraries( server ${OpenCV_LIBS} ${Boost_LIBRARIES} ${FFMPEG_LIBRARY_avcodec} ${FFMPEG_LIBRARY_avutil} ${DXGI_LIBRARY1} ${DXGI_LIBRARY2})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)