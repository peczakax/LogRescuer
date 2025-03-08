cmake_minimum_required(VERSION 3.10)
project(LogRescuer VERSION 1.0)

# Add BUILD_SHARED_LIBS option
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WITH_BROTLI "Enable Brotli support" ON)
option(WITH_ZLIB "Enable ZLIB support" ON)
option(WITH_ZSTD "Enable ZStandard support" ON)
option(BUILD_TESTS "Build test suite" ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Define base sources
set(SOURCES    
    src/HashUtils.cpp
    src/CompressorFactory.cpp
    src/IO.cpp
    src/FileCompressor.cpp
    src/ThreadPool.cpp
)

find_package(OpenSSL REQUIRED)

# Optional compression libraries
if(WITH_ZLIB)
    find_package(ZLIB)
    if(ZLIB_FOUND)
        add_compile_definitions(HAVE_ZLIB)
        list(APPEND SOURCES src/ZlibCompressor.cpp)
    endif()
endif()

if(WITH_BROTLI)
    find_package(Brotli)
    if(Brotli_FOUND)
        add_compile_definitions(HAVE_BROTLI)
        list(APPEND SOURCES src/BrotliCompressor.cpp)
    endif()
endif()

if(WITH_ZSTD)
    find_package(ZSTD)
    if(ZSTD_FOUND)
        add_compile_definitions(HAVE_ZSTD)
        list(APPEND SOURCES src/ZStandardCompressor.cpp)
    endif()
endif()

if(NOT (ZLIB_FOUND OR Brotli_FOUND OR ZSTD_FOUND))
    message(FATAL_ERROR "No compression libraries found. At least one of Brotli, ZLIB, or ZStandard is required.")
endif()

# Create a library for the core functionality
add_library(logrescuer_lib ${SOURCES})
target_include_directories(logrescuer_lib PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(logrescuer_lib PROPERTIES OUTPUT_NAME logrescuer)

# Link dependencies to the library
target_link_libraries(logrescuer_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)

if(ZLIB_FOUND)
    target_link_libraries(logrescuer_lib PUBLIC ZLIB::ZLIB)
endif()

if(Brotli_FOUND)
    target_link_libraries(logrescuer_lib PUBLIC brotli::brotlienc
                                                brotli::brotlidec
                                                brotli::brotlicommon)
endif()

if(ZSTD_FOUND)
    target_link_libraries(logrescuer_lib PUBLIC zstd::libzstd_static)
endif()

# Main executable
add_executable(logrescuer apps/logrescuer.cpp)
target_link_libraries(logrescuer PRIVATE logrescuer_lib)

# Testing configuration
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    
    # Create the test executable for HashUtils
    add_executable(test_hashutils tests/test_HashUtils.cpp)
    target_link_libraries(test_hashutils PRIVATE logrescuer_lib GTest::GTest GTest::Main)

    # Create the test executable for FileCompressor
    add_executable(test_filecompressor tests/test_FileCompressor.cpp)
    target_link_libraries(test_filecompressor PRIVATE logrescuer_lib GTest::GTest GTest::Main)

    # Create the test executable for IO
    add_executable(test_io tests/test_IO.cpp)
    target_link_libraries(test_io PRIVATE logrescuer_lib GTest::GTest GTest::Main)
    
    # Register the test with CTest
    add_test(NAME FileCompressorTests COMMAND test_filecompressor)
    add_test(NAME HashUtilsTests COMMAND test_hashutils)
    add_test(NAME IOTests COMMAND test_io)
endif()

# Installation rules
install(TARGETS logrescuer_lib logrescuer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")

# Make README.md installation optional
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES README.md DESTINATION share/logrescuer)
endif()
