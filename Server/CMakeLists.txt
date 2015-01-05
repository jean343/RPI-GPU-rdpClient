cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}")

project( server )

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package( Boost REQUIRED COMPONENTS thread )
if(Boost_FOUND)
    message("Boost found!")
endif()

LINK_DIRECTORIES( "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.5/lib/Win32")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIR})
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
		set (FFMPEG_LIBRARIES "${FFMPEG_LIBRARY_avcodec};${FFMPEG_LIBRARY_avutil}")
		message("FFMPEG found!")
	endif()
endif()

#Files for WDDM
find_package( DirectX )
include_directories(${DXGI_INCLUDES})
include_directories( ${Boost_INCLUDE_DIRS} )
include_directories( "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v6.5/include/" )
set (CUDA_LIBRARIES "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v6.5/lib/Win32/cuda.lib;C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v6.5/lib/Win32/cudart.lib")

#files for NVEncoder
set (NVENCODER_FOUND 1)
SET(NVENCODER_SOURCE
  NvEncoder/NvHWEncoder.cpp
)
SET(NVENCODER_HEADERS
  NV_encoding.hpp
  NvEncoder/NvEncoder.h
  NvEncoder/NvHWEncoder.h
  color_conversion.cu
)

add_executable( server server.cpp config.h ${NVENCODER_SOURCE} ${NVENCODER_HEADERS} )


target_link_libraries( server ${Boost_LIBRARIES} ${FFMPEG_LIBRARIES} ${DXGI_LIBRARIES} ${CUDA_LIBRARIES})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)