cmake_minimum_required(VERSION 2.8)
project( graphics )

include(TestCXXAcceptsFlag)
if(NOT DISABLE_GXX0XCHECK)
  # try to use compiler flag -std=c++11
  check_cxx_accepts_flag("-std=c++11" CXX_FLAG_CXX11)
endif(NOT DISABLE_GXX0XCHECK)
if(NOT CXX_FLAG_CXX11)
    message(FATAL_ERROR "Compiler must support c++11")
endif()

if( CMAKE_SIZEOF_VOID_P EQUAL 8 ) # 64 bit architecture detected
    set(ARCH_64 True)
endif()

add_definitions(-Dunix)
add_definitions(-Dlinux)
if( ARCH_64 )
        message( "Detected 64bit build arch.. setting 32bit build options")
        if( COMMAND SET_PROPERTY )
            SET_PROPERTY(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS 0)
        else()
            message( "Command SET_PROPERTY not found. do you have cmake >= 2.6 ?" )
        endif()

        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" )
        set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32" )
        set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32" )
        set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32" )
        set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32" )

endif()

set( CMAKE_CXX_FLAGS "-std=c++11" )

find_package(OpenGL REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(GTK2 REQUIRED gtk)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Freetype REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
find_package(Curses REQUIRED)

# special case for X11 libs
if(NOT X11_ICE_FOUND)
   MESSAGE(FATAL_ERROR "\nThe X11 Inter-Client Exchange (ICE) Library could not be found.\nCannot continue. Please install libICE.\n")
endif(NOT X11_ICE_FOUND)

if(NOT X11_SM_FOUND)
  MESSAGE(FATAL_ERROR "\nThe X11 Session Management (SM) Library could not be found.\nCannot continue. Please install libSM.\n")
endif(NOT X11_SM_FOUND)

if(NOT X11_Xext_FOUND)
    MESSAGE(FATAL_ERROR "\nThe X.Org X11 libXext runtime library could not be found.\nCannot continue. Please install libXext.\n")
endif()

SET(SOURCES
    g_src/basics.cpp g_src/command_line.cpp g_src/enabler.cpp g_src/enabler_input.cpp
	g_src/files.cpp g_src/find_files_posix.cpp g_src/graphics.cpp g_src/init.cpp
	g_src/interface.cpp g_src/keybindings.cpp g_src/KeybindingScreen.cpp
	g_src/random.cpp g_src/renderer_offscreen.cpp g_src/resize++.cpp
	g_src/textures.cpp g_src/textlines.cpp g_src/ttf_manager.cpp g_src/ViewBase.cpp
	g_src/win32_compat.cpp g_src/music_and_sound_openal.cpp
)

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${SDL_INCLUDE_DIR}
    ${SDL_TTF_INCLUDE_DIRS}
    ${SDL_IMAGE_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${GTK2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
    ${CURSES_INCLUDE_DIR}
)

add_library( graphics SHARED ${SOURCES} )

if( ARCH_64 )
    set_target_properties( graphics PROPERTIES COMPILE_FLAGS "-std=c++11 -m32" LINK_FLAGS "-m32" )
else()
    set_target_properties( graphics PROPERTIES COMPILE_FLAGS "-std=c++11" LINK_FLAGS "" )
endif()

target_link_libraries( graphics
    ${OPENGL_LIBRARY}
    ${SDL_LIBRARY}
    ${SDL_TTF_LIBRARIES}
    ${SDL_IMAGE_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${GTK2_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${X11_LIBRARIES}
    ${CURSES_LIBRARIES}
)
