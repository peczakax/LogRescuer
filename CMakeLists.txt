cmake_minimum_required(VERSION 3.10)
project(LogRescuer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Compression library options
option(WITH_BROTLI "Enable Brotli support" ON)
option(WITH_ZLIB "Enable ZLIB support" ON)
option(WITH_ZSTD "Enable ZStandard support" ON)
option(BUILD_TESTS "Build test suite" ON)

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

# Main executable
add_executable(logrescuer apps/logrescuer.cpp ${SOURCES})
target_include_directories(logrescuer PRIVATE src)
target_include_directories(logrescuer PUBLIC include)

# Link libraries
target_link_libraries(logrescuer PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Link libraries conditionally
if(ZLIB_FOUND)
    target_link_libraries(logrescuer PRIVATE ZLIB::ZLIB)
endif()

if(Brotli_FOUND)
    target_link_libraries(logrescuer PRIVATE Brotli::brotlienc
                                             Brotli::brotlidec
                                             Brotli::brotlicommon)
endif()

if(ZSTD_FOUND)
    target_link_libraries(logrescuer PRIVATE ZSTD::ZSTD)
endif()

# Testing configuration
if(BUILD_TESTS)
    # Try to find pre-installed GTest, fallback to FetchContent if not found
    find_package(GTest REQUIRED)
    
    # Enable testing for CTest integration
    enable_testing()
    
        
    # Create a library from the main sources for testing
    add_library(logrescuer_lib STATIC ${SOURCES})
    target_include_directories(logrescuer_lib PUBLIC src include)
    target_link_libraries(logrescuer_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    
    # Link compression libraries to the library
    if(ZLIB_FOUND)
        target_link_libraries(logrescuer_lib PRIVATE ZLIB::ZLIB)
    endif()
    if(Brotli_FOUND)
        target_link_libraries(logrescuer_lib PRIVATE Brotli::brotlienc
                                                     Brotli::brotlidec
                                                     Brotli::brotlicommon)
    endif()
    if(ZSTD_FOUND)
        target_link_libraries(logrescuer_lib PRIVATE ZSTD::ZSTD)
    endif()
    
    # Create the test executable for HashUtils
    add_executable(test_hashutils tests/test_HashUtils.cpp)
    target_include_directories(test_hashutils PRIVATE src include)
    
    # Create the test executable for FileCompressor
    add_executable(test_filecompressor tests/test_FileCompressor.cpp)
    target_include_directories(test_filecompressor PRIVATE src include)

    # Create the test executable for IO
    add_executable(test_io tests/test_IO.cpp)
    target_include_directories(test_io PRIVATE src include)    
        
    # Link with our library and Google Test
    target_link_libraries(test_hashutils PRIVATE logrescuer_lib GTest::GTest GTest::Main)
    target_link_libraries(test_filecompressor PRIVATE logrescuer_lib GTest::GTest GTest::Main)
    target_link_libraries(test_io PRIVATE logrescuer_lib GTest::GTest GTest::Main)
    
    # Register the test with CTest
    add_test(NAME FileCompressorTests COMMAND test_filecompressor)
    add_test(NAME HashUtilsTests COMMAND test_hashutils)
    add_test(NAME IOTests COMMAND test_io)

    # Add more test executables here as needed
endif()
