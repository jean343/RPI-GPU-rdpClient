cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}")

project( server )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# BOOST
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package( Boost REQUIRED COMPONENTS thread )
if(Boost_FOUND)
    message("Boost found!")
endif()
include_directories(${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

# FFMPEG
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

# DXGI and CUDA
find_package(CUDA)
if(CUDA_FOUND)
    message("CUDA found!")
endif()
find_package( DirectX )
include_directories(${DXGI_INCLUDES})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CUDA_TOOLKIT_INCLUDE})
CUDA_ADD_LIBRARY(cudalib STATIC
    color_conversion.h
    color_conversion.cu
    OPTIONS -arch sm_20
)
TARGET_LINK_LIBRARIES(cudalib ${CUDA_LIBRARIES})
set (CUDA_LINK_LIBRARIES "${CUDA_CUDA_LIBRARY};${CUDA_CUDART_LIBRARY};cudalib")

#files for NVEncoder
set (NVENCODER_FOUND 1)
SET(NVENCODER_SOURCE
  NvEncoder/NvHWEncoder.cpp
)
SET(NVENCODER_HEADERS
  NV_encoding.hpp
  NvEncoder/NvEncoder.h
  NvEncoder/NvHWEncoder.h
)

add_executable( server server.cpp config.h ${NVENCODER_SOURCE} ${NVENCODER_HEADERS} )


target_link_libraries( server ${Boost_LIBRARIES} ${FFMPEG_LIBRARIES} ${DXGI_LIBRARIES} ${CUDA_LINK_LIBRARIES})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)