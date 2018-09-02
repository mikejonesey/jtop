cmake_minimum_required(VERSION 2.8)
project(jtop)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

find_package (Threads REQUIRED)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fPIE -pie")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -fPIE -pie")

#set(CMAKE_C_STANDARD 99)
macro(use_c99)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
            set (CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")
        endif ()
    else ()
        set (CMAKE_C_STANDARD 99)
    endif ()
endmacro(use_c99)
use_c99()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DESTDIR})

set(SOURCE_FILES jtop.c jtop.h topWindow.c topWindow.h procData.c procData.h jvmData.c jvmData.h stackWindow.c stackWindow.h bufferFun.c bufferFun.h classcpuWindow.c classcpuWindow.h)
add_executable(jtop ${SOURCE_FILES})
target_link_libraries(jtop ${CURSES_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})


#add_custom_target(install_${PROJECT_NAME}
#        "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target install
#        DEPENDS ${PROJECT_NAME}
#        COMMENT "Installing ${PROJECT_NAME}")

#add_custom_target(build_${PROJECT_NAME}
#        "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target build
#        DEPENDS ${PROJECT_NAME}
#        COMMENT "Building ${PROJECT_NAME}")

#add_custom_target(build
#        "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target build
#        COMMENT "Building ${PROJECT_NAME}")

#add_custom_target(install_${PROJECT_NAME}
#        $(MAKE) install
#        DEPENDS ${PROJECT_NAME}
#        COMMENT "Installing ${PROJECT_NAME}")
