from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import check_min_cppstd
from conan.tools.files import copy
import os

class LogRescuerConan(ConanFile):
    name = "logrescuer"
    version = "1.0.0"
    description = "A log compression tool that efficiently compresses directories of log files while preserving directory structure and eliminating duplicate content"
    license = "Proprietary" 
    url = "https://github.com/peczakax/logrescuer"
    homepage = "https://github.com/peczakax/logrescuer"
    topics = ("logs", "compression", "archiving")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_brotli": [True, False],
        "with_zlib": [True, False],
        "with_zstd": [True, False],
        "build_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_brotli": True,
        "with_zlib": True,
        "with_zstd": True,
        "build_tests": True,
        "openssl/*:shared": False,
        "zlib/*:shared": False,
        "brotli/*:shared": False,
        "zstd/*:shared": False,
    }
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "apps/*", "cmake/*", "README.md", "tests/*"
    test_package_folder = "tests"

    def validate(self):
        # Ensure we have at least one compression algorithm enabled
        if not any([self.options.with_brotli, self.options.with_zlib, self.options.with_zstd]):
            raise ValueError("At least one compression algorithm must be enabled")
        check_min_cppstd(self, "17")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # OpenSSL is always required for hashing functionality
        self.requires("openssl/3.1.3")
        
        # Add optional compression libraries based on options
        if self.options.with_brotli:
            self.requires("brotli/1.1.0")
        if self.options.with_zlib:
            self.requires("zlib/1.3")
        if self.options.with_zstd:
            self.requires("zstd/1.5.5")
    
    def build_requirements(self):
        # Add gtest as a build requirement if tests are enabled
        if self.options.build_tests:
            self.test_requires("gtest/1.14.0")

    def build(self):
        cmake = CMake(self)
        
        # Map Conan options to CMake options
        cmake_options = {
            "WITH_BROTLI": self.options.with_brotli,
            "WITH_ZLIB": self.options.with_zlib,
            "WITH_ZSTD": self.options.with_zstd,
            "BUILD_TESTS": self.options.build_tests
        }
        
        # Configure, build, and test if enabled
        cmake.configure(variables=cmake_options)
        cmake.build()
        
        if self.options.build_tests:
            cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        
        # Copy license files and documentation
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        copy(self, "README.md", self.source_folder, os.path.join(self.package_folder, "licenses"))
        
        # Include header files
        copy(self, "*.h", src=os.path.join(self.source_folder, "include"), 
             dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.inl", src=os.path.join(self.source_folder, "include"), 
             dst=os.path.join(self.package_folder, "include"))

    def package_info(self):
        # Define libraries to link against
        self.cpp_info.libs = ["logrescuer"]
        
        # Define any compile definitions needed by consumers
        if self.options.with_brotli:
            self.cpp_info.defines.append("HAVE_BROTLI")
        if self.options.with_zlib:
            self.cpp_info.defines.append("HAVE_ZLIB")
        if self.options.with_zstd:
            self.cpp_info.defines.append("HAVE_ZSTD")
