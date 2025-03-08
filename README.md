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

## Legal Notice

This work constitutes an original work within the meaning of Copyright and Related Rights Act and may not be used for purposes other than evaluating the author's potential within the recruitment process in which they are participating. Commercial use without the author's consent is prohibited.

