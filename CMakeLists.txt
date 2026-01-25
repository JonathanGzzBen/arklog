cmake_minimum_required(VERSION 3.11)

project(arklog LANGUAGES C)

include(CheckCCompilerFlag)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # Used for nanosleep()

set(custom_compiler_flags)
if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
        list(APPEND custom_compiler_flags
            -pedantic
            -Wall
            -Wextra
            -Werror
            -Wstrict-prototypes
            -Wwrite-strings
            -Wshadow
            -Winit-self
            -Wcast-align
            -Wformat=2
            -Wmissing-prototypes
            -Wstrict-overflow=2
            -Wcast-qual
            -Wundef
            -Wswitch-default
            -Wconversion
            -Wc++-compat
            -fstack-protector-strong
            -Wcomma
            -Wdouble-promotion
            -Wparentheses
            -Wformat-overflow
            -Wunused-macros
            -Wmissing-variable-declarations
            -Wused-but-marked-unused
            -Wswitch-enum
        )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        # Disable warning c4001 - nonstandard extension 'single line comment' was used
        # Define _CRT_SECURE_NO_WARNINGS to disable deprecation warnings for "insecure" C library functions
        list(APPEND custom_compiler_flags
            /GS
            /Za
            /sdl
            /W4
            /wd4001
            /D_CRT_SECURE_NO_WARNINGS
        )
    endif()

# Enable sanitizers
option(USE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(USE_SANITIZERS)
    if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU"))
        list(APPEND custom_compiler_flags
                -fno-omit-frame-pointer
                -fno-sanitize-merge
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=float-cast-overflow
                -fsanitize-address-use-after-scope
                -fsanitize=integer
                -fno-sanitize-recover
                )
    elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        list(APPEND custom_compiler_flags
                -fsanitize=address
                )
    endif()
endif()

add_library(arklog_lib src/arklog.c src/ring_buffer.c)
target_include_directories(arklog_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# apply custom compiler flags
foreach(compiler_flag ${custom_compiler_flags})
    #remove problematic characters
    string(REGEX REPLACE "[^a-zA-Z0-9]" "" current_variable ${compiler_flag})

    CHECK_C_COMPILER_FLAG(${compiler_flag} "FLAG_SUPPORTED_${current_variable}")
    if (FLAG_SUPPORTED_${current_variable})
        add_compile_options(${compiler_flag})
        add_link_options(${compiler_flag})
    endif()
endforeach()

add_executable(arklog_test tests/tests.c tests/ring_buffer_tests.c tests/logger_tests.c)
target_link_libraries(arklog_test PRIVATE arklog_lib)
