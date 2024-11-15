cmake_minimum_required(VERSION 3.29)

project(SimpleSnapFS C CXX)

enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

# Universal compiler and linker flags
set(compiler_options
        # Warnings and diagnostics
        -Wall                                # Enable common warnings
        -Wextra                              # Enable extra warnings
        -Wpedantic                           # Strict compliance with the standard
        -Wunused                             # Warn about unused variables, functions, etc.
        -Wuninitialized                      # Warn if variables are used uninitialized
        -fdiagnostics-show-option            # Show which option triggered the warning
        -fdiagnostics-color=always           # Enable colored diagnostics for better readability

        -std=c++20

        # Debugging and stack protection
        -g3                                  # Maximum debug information, including macro expansions
        -O0                                  # No optimization, easier for debugging
        -fstack-usage                        # Generate stack usage info for each function
        -fstack-protector-all                # Protect all functions with a stack canary to prevent stack overflow attacks
        -D_FORTIFY_SOURCE=2                  # Buffer overflow detection on safer libc functions (e.g., memcpy)
        -gdwarf-4

        # Sanitize memory and thread issues
        -fsanitize=address                   # Detect illegal memory access such as buffer overflows and use-after-free
        -fsanitize=undefined                 # Detect undefined behavior like integer overflows and null dereferencing
        #        -fsanitize=thread                    # Ensure thread safety by detecting data races

        # Linker options
        #        -Wl,--verbose                        # Verbose linker output to show linking process details
        -flto                                # Enable Link-Time Optimization, but keep it compatible with debug builds
        -fprofile-arcs -ftest-coverage -fPIC -pie
)

set(linker_options
        # Linker options for memory safety, thread safety, and verbose debugging
        -fuse-ld=gold                           # Use the gold linker for faster linking (optional, depending on system availability)
        -Wl,--no-omagic                         # Prevent the generation of object files in memory; useful for debugging
        -Wl,--as-needed                         # Only link libraries that are actually needed to reduce binary size
        -Wl,--fatal-warnings                    # Treat all linker warnings as errors to catch issues early
        -Wl,-z,relro                            # Read-only relocations to prevent certain memory exploits (optional)
        -Wl,-z,now                              # Fully resolve all symbols during the link time for extra safety
        -Wl,-z,noexecstack                      # Prevent execution of code on the stack (security hardening)
        -Wl,-z,defs                             # Ensure all symbols are defined, and prevent undefined symbols
        -Wl,-O1                                 # Optimize the linking process while maintaining debuggability

        -std=c++20

        -gdwarf-4

        # AddressSanitizer and ThreadSanitizer linking (to ensure proper linking of sanitizers)
        -fsanitize=address                      # Link the AddressSanitizer runtime for memory integrity
        #        -fsanitize=thread                       # Link the ThreadSanitizer runtime for thread safety
        -fsanitize=undefined                    # Link the UndefinedBehaviorSanitizer for detecting undefined behavior

        # Stack protection and LTO options
        -fstack-protector-all                   # Link with stack protection for all functions
        -flto                                   # Enable link-time optimization (can be disabled during debug for clarity)
        -fprofile-arcs -ftest-coverage -fPIC -pie
)

if ("${SANITIZER_CHECK}" STREQUAL "True")
    add_compile_options(${compiler_options})
    add_link_options(${linker_options})
else ()
    add_compile_options(-std=c++20 -g3 -O0 -gdwarf-4)
    add_link_options(-Wl,-O1 -gdwarf-4)
endif ()

add_compile_definitions(CMAKE_BINARY_DIR=\"${CMAKE_BINARY_DIR}\")
add_compile_definitions(CMAKE_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\")

function(add_unit_test
        TEST_NAME                                   # test name, also the test executable file name
        TEST_MAINF_FILE                             # location of the file that is used to test functionality (contains int main(int, char**))
        TEST_LIBRARY                                # the library that is being tested
        )
    add_executable(${TEST_NAME} ${TEST_MAINF_FILE})
    target_link_libraries(${TEST_NAME} PUBLIC ${TEST_LIBRARY})
    add_test(NAME unit_test_${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(unit_test_${TEST_NAME} PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_odr_violation=0")
endfunction()

include_directories(src/include)

# Debug support
add_library(fs_debug SHARED
        src/include/debug.h
        src/debug/runtime_error.cpp
        src/debug/stack_frame_tracing.cpp
)
add_unit_test(fs_debug_test src/tests/fs_debug_test.cpp fs_debug)

# filesystem
find_package(OpenSSL REQUIRED)
add_library(simplesnapfs SHARED
        src/simplesnapfs/bitmap.cpp
        src/simplesnapfs/checksum.cpp
        src/simplesnapfs/block_io.cpp

        src/include/simplesnapfs.h
        src/include/bitmap.h
        src/include/checksum.h
        src/include/block_io.h
)
target_link_libraries(simplesnapfs PUBLIC fs_debug OpenSSL::SSL OpenSSL::Crypto)

# utility helper library
add_library(utility STATIC
        src/include/utility.h
        src/utils/utility.cpp
)

# utility: mkfs.simplesnapfs
add_executable(mkfs.simplesnapfs
        src/utils/mkfs.simplesnapfs.cpp
)
target_link_libraries(mkfs.simplesnapfs PUBLIC simplesnapfs fs_debug utility)
