cmake_minimum_required (VERSION 2.6)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Some lib stuff, probably unneeded, but I don't know cmake too well yet...
if (WIN32)
	set(LIB_PREFIX )
	set(LIB_POSTFIX .lib)
else ()
	set(LIB_PREFIX lib)
	set(LIB_POSTFIX .so)
endif ()

# Some options:
set(INSTALL_RUNTIMES_TO "/usr/bin" CACHE STRING "Where install executables.")
set(INSTALL_LIBS_TO "/usr/lib" CACHE STRING "Where to install libs.")

set(ORYX_INCLUDE_DIR "../OryxEngine/src" CACHE STRING "Where to look for Oryx includes.")
set(ORYX_LIB_DIR "/usr/lib" CACHE STRING "Where to look for Oryx libs.")
set(ORYX_SUBSYSTEM_INCLUDE_DIR "../OryxEngine/src" CACHE STRING "Where to look for Oryx Subsystem includes.")
set(ORYX_SUBSYSTEM_LIB_DIR "/usr/lib" CACHE STRING "Where to look for Oryx Subsystems.")

set(BOOST_INCLUDE_DIR "/usr/include" CACHE STRING "Where to look for boost includes.")
set(BOOST_LIB_DIR "/usr/lib" CACHE STRING "Where to look for boost libs.")

# all the oryx libs
set(ORYX_LIB ${ORYX_LIB_DIR}/${LIB_PREFIX}OryxEngine${LIB_POSTFIX})
set(OGRE_LIB ${ORYX_LIB_DIR}/${LIB_PREFIX}OgreSubsystem${LIB_POSTFIX})
set(AL_LIB ${ORYX_LIB_DIR}/${LIB_PREFIX}ALSubsystem${LIB_POSTFIX})
set(GUI_LIB ${ORYX_LIB_DIR}/${LIB_PREFIX}GUISubsystem${LIB_POSTFIX})
set(BULLET_LIB ${ORYX_LIB_DIR}/${LIB_PREFIX}BulletSubsystem${LIB_POSTFIX})
set(OIS_LIB ${ORYX_LIB_DIR}/${LIB_PREFIX}OISSubsystem${LIB_POSTFIX})

set(BOOST_THREAD_LIB ${BOOST_LIB_DIR}/${LIB_PREFIX}boost_thread${LIB_POSTFIX})

include_directories("${ORYX_INCLUDE_DIR}")
include_directories("${ORYX_SUBSYSTEM_INCLUDE_DIR}")
include_directories("src")

# copy art assets and scripts over to the bin dir
file(COPY ./media DESTINATION ./)

add_subdirectory(src)
