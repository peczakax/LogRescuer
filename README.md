# Legal Notice

This work constitutes an original work within the meaning of Copyright and Related Rights Act and may not be used for purposes other than evaluating the author's potential within the recruitment process in which they are participating. Commercial use without the author's consent is prohibited.

# LogRescuer

LogRescuer is a log compression tool that efficiently compresses directories of log files while preserving directory structure and eliminating duplicate content.

## Features

LogRescuer incorporates several advanced capabilities to handle log file management:

- **Recursive Directory Archiving**: Traverses and preserves complex directory hierarchies, capturing the complete file system context of your logs.

- **Content-Aware Deduplication**: Uses cryptographic hashing to identify bit-identical files, storing each unique file content only once regardless of how many copies exist.

- **Structural Integrity**: Maintains the exact original directory structure during both compression and extraction operations, ensuring log analysis tools continue to function correctly.

- **Algorithm Flexibility**: Supports three industry-standard compression implementations:
  - **Brotli**: Delivers exceptional compression ratios at the cost of slightly higher CPU usage
  - **Zlib**: Provides wide platform compatibility with solid compression performance
  - **Zstd**: Balances speed and compression ratio

- **Concurrent Processing**: Implements a custom thread pool architecture that scales with available CPU cores, accelerating file operations on multi-core systems.

- **Data Verification**: Employs SHA-256 hashing to verify extracted file integrity, guaranteeing that decompressed logs match the original.

## Installation

### Dependencies

**Note:** At least one compression library must be installed for LogRescuer to function properly. The program requires at minimum one of: Brotli, Zlib, or Zstd.

Additionally, LogRescuer requires OpenSSL3 for secure cryptographic operations, specifically for SHA-256 hash computation used in file deduplication and integrity verification.

#### For Ubuntu/Debian (22.04+)
```
# Compression libraries
sudo apt-get install libbrotli-dev
sudo apt-get install libzstd-dev
sudo apt-get install zlib1g-dev

# Cryptography requirement
sudo apt-get install libssl-dev
```

#### For older Ubuntu/Debian versions
OpenSSL3 may need to be installed from source or from a PPA:
```
# Add the PPA for OpenSSL3
sudo add-apt-repository ppa:ubuntu-toolchain-r/ppa
sudo apt update
sudo apt install libssl3-dev
```

#### For RHEL/CentOS (8+)
```
# Compression libraries
sudo dnf install brotli-devel
sudo dnf install libzstd-devel
sudo dnf install zlib-devel

# Cryptography requirement
sudo dnf install openssl-devel
```

## Project Structure

The project follows a standard structure with the tests directory containing test package code:

```
logrescuer/
├── apps/        # Application source files
├── cmake/       # CMake modules and configuration
├── include/     # Public API headers
├── src/         # Library source files
└── tests/       # Test package for Conan and unit tests
```

## Building with CMake

LogRescuer can be built directly with CMake. This approach requires manually installing the dependencies beforehand.

### Prerequisites

Ensure you have CMake installed (version 3.10 or higher recommended):

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# RHEL/CentOS
sudo dnf install cmake
```

### Basic Build Steps

1. Clone the repository:
```bash
git clone https://github.com/yourusername/logrescuer.git
cd logrescuer
```

2. Create a build directory and navigate to it:
```bash
mkdir build && cd build
```

3. Configure the project:
```bash
cmake ..
```

4. Build the project:
```bash
cmake --build .
```

### CMake Configuration Options

LogRescuer supports several build options that can be customized:

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | OFF | Build as a shared library instead of static |
| `WITH_BROTLI` | ON | Include Brotli compression support |
| `WITH_ZLIB` | ON | Include Zlib compression support |
| `WITH_ZSTD` | ON | Include Zstd compression support |
| `BUILD_TESTS` | ON | Build and run unit tests |

To use these options when configuring:

```bash
cmake -DWITH_BROTLI=ON -DWITH_ZLIB=OFF -DWITH_ZSTD=ON -DBUILD_TESTS=ON ..
```

**Note:** At least one compression algorithm (Brotli, Zlib, or Zstd) must be enabled.

### Example CMake Build Scenarios

#### Debug Build with Tests
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
cmake --build .
```

#### Release Build with Only Zstd Support
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_BROTLI=OFF -DWITH_ZLIB=OFF -DWITH_ZSTD=ON ..
cmake --build .
```

#### Installing to System (requires appropriate permissions)
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
sudo cmake --install .
```

### Running Tests

If you've built with tests enabled:

```bash
cd build
ctest
```

Or to run tests with more verbose output:

```bash
ctest --verbose
```

## Building with Docker

You can build and run LogRescuer using Docker to avoid installing dependencies directly on your system:

### Building the Docker Image

Place the Dockerfile in the root of your LogRescuer project and build the image:

```
docker build -t logrescuer .
```

### Running with Docker

Once built, you can run the application with Docker:

```bash
# Show help
docker run --rm logrescuer

# Compress logs (mounting volumes to access files)
docker run --rm -v /path/to/logs:/logs -v /path/to/output:/output logrescuer compress /logs /output/archive.bin

# Decompress logs
docker run --rm -v /path/to/output:/output -v /path/to/extract:/extract logrescuer decompress /extract /output/archive.bin
```

This creates a self-contained environment with all required dependencies to build and run LogRescuer.

## Building with Conan

LogRescuer uses [Conan](https://conan.io/) as its package manager to handle dependencies and build processes. This enables consistent builds across different platforms and development environments.

### Installing Conan

If you don't have Conan installed yet:

```bash
pip install conan
```

For detailed installation instructions, visit the [official Conan documentation](https://docs.conan.io/en/latest/installation.html).

### Building LogRescuer using Conan

1. Clone the repository:
```bash
git clone https://github.com/yourusername/logrescuer.git
cd logrescuer
```

2. Install dependencies and generate build files:
```bash
conan install . --build=missing
```

3. Build the project:
```bash
conan build .
```

The `--build=missing` flag is crucial as it tells Conan to build any dependencies that don't have pre-built binaries available for your system. Without this flag, you might encounter errors about missing packages.

### Conan Configuration Options

LogRescuer supports several build options that can be customized:

| Option | Default | Description |
|--------|---------|-------------|
| `shared` | False | Build as a shared library instead of static   |
| `fPIC` | True | Generate position-independent code (Linux/macOS) |
| `with_brotli` | True | Include Brotli compression support        |
| `with_zlib` | True | Include Zlib compression support            |
| `with_zstd` | True | Include Zstd compression support            |
| `build_tests` | True | Build and run unit tests                  |

To use these options when building:

```bash
conan install . -o with_zstd=True -o with_brotli=False -o build_tests=True --build=missing
conan build .
```

**Note:** At least one compression algorithm (Brotli, Zlib, or Zstd) must be enabled.

### Build Profiles

You can create custom build profiles to easily switch between different configurations:

1. Create a profile file (e.g., `debug_profile`):
```ini
[settings]
os=Linux
compiler=gcc
compiler.version=11
compiler.libcxx=libstdc++11
build_type=Debug

[options]
logrescuer:build_tests=True
logrescuer:with_brotli=True
logrescuer:with_zlib=False
logrescuer:with_zstd=True
```

2. Use the profile:
```bash
conan install . -pr debug_profile --build=missing
conan build .
```

### Example Build Scenarios

#### Minimal Build (with only Zlib)
```bash
conan install . -o with_brotli=False -o with_zlib=True -o with_zstd=False --build=missing
conan build .
```

#### Development Build with Tests
```bash
conan install . -o build_tests=True -s build_type=Debug --build=missing
conan build .
```

#### Production Release Build
```bash
conan install . -s build_type=Release --build=missing
conan build .
```

#### Troubleshooting Missing Dependencies

If you encounter errors about missing dependencies like:
```
ERROR: Missing binary: dependency/version:hash
```

You can resolve this by adding the `--build=missing` flag to your Conan install command:
```bash
conan install . --build=missing
```

Or to build a specific missing dependency:
```bash
conan install . --build=dependency_name
```

## Usage

```
LogRescuer - A time machine log compression and archival tool.

Usage: logrescuer <command> <dir> <archive_file> [options]

Commands:
  compress    - Create a compressed archive.
  decompress  - Extract an archive.

Options:
  -c, --compression    Optionally specify a compression algorithm: [brotli, zlib, zstd] (default depends on build)
  -h, --help           Print this help message.
```

### Examples

The following examples demonstrate typical LogRescuer usage patterns:

**Archiving Log Collections**

Create a compressed archive of logs using Brotli compression (highest ratio):
```
logrescuer compress /var/logs log_archive --compression=brotli
```

Archive logs with Zstd for faster compression:
```
logrescuer compress /var/logs log_archive -c=zstd
```

**Extracting Archives**

Restore a complete log collection to a target directory:
```
logrescuer decompress /tmp/logs log_archive
```

## Docker Usage

You can run LogRescuer using Docker to avoid installing dependencies directly on your system:

### Building the Docker Image

Place the Dockerfile in the root of your LogRescuer project and build the image:

```
docker build -t logrescuer .
```

### Running with Docker

Once built, you can run the application with Docker:

```bash
# Show help
docker run --rm logrescuer

# Compress logs (mounting volumes to access files)
docker run --rm -v /path/to/logs:/logs -v /path/to/output:/output logrescuer compress /logs /output/archive.bin

# Decompress logs
docker run --rm -v /path/to/output:/output -v /path/to/extract:/extract logrescuer decompress /extract /output/archive.bin
```

This creates a self-contained environment with all required dependencies to build and run LogRescuer.

## How It Works

Under the hood, LogRescuer operates through several key stages:

1. **Directory Scanning**: First, the tool walks through your source directory structure, mapping all regular files and recording their relative paths for later reconstruction.

2. **Hash-Based Deduplication**: Instead of naively compressing every file, LogRescuer calculates SHA-256 hashes of each file using a thread pool for performance. These hashes act as digital fingerprints, letting us detect when two or more files contain identical data.

3. **Smart File Grouping**: Files are sorted into two categories - "unique" (first occurrence of a specific content hash) and "duplicate" (additional occurrences). This separation is crucial for the space-saving mechanism.

4. **Streaming Compression**: LogRescuer processes each unique file by streaming it through the selected compression engine (Brotli, Zlib, or Zstd) directly into the archive. This streaming approach minimizes memory overhead even with large files.

5. **Metadata Tracking**: For each file, we track essential metadata including relative path, content hash, compressed size, and offset position within the archive. Importantly, duplicate files store a reference to the original content rather than redundant data.

6. **Verified Extraction**: During decompression, the tool rebuilds your directory structure exactly as it was. Each extracted file undergoes hash verification to ensure data integrity, and duplicate files are reconstructed from their single compressed source.

Multi-threading is used throughout the pipeline with carefully placed mutex locks to ensure thread safety while maximizing parallel processing opportunities on modern multi-core systems.

## Benefits

LogRescuer delivers several technical advantages over conventional compression tools:

- **Storage Optimization**: By identifying bit-identical files via SHA-256 hashing, the system stores only one copy of duplicate content, reducing archive size in environments with many similar logs.

- **Rapid Data Recovery**: The indexed archive structure allows for quick targeted extraction of specific files without decompressing the entire archive, speeding up log retrieval during investigations.

- **Cryptographic Verification**: Every extracted file's content is validated against its original SHA-256 hash, ensuring byte integrity even after transfer across networks or storage systems.

- **Hierarchical Preservation**: The tool reconstructs the exact directory structure during extraction, maintaining proper context for log analysis tools or organizational requirements.

- **Parallelized Processing**: A thread pool implementation leverages multi-core CPUs to compress, hash, and extract files concurrently, reducing processing time for large log collections.

- **Flexible Compression Options**: Support for industry-standard algorithms (Brotli, Zstd, Zlib) lets prioritize either compression ratio, speed, or compatibility based on specific requirements.
