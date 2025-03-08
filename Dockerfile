FROM ubuntu:22.04

# Avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libbrotli-dev \
    libzstd-dev \
    zlib1g-dev \
    git \
    googletest \
    libgtest-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy project files
COPY . .

# Create build directory and build the application
RUN mkdir -p build && cd build \
    && cmake .. \
    && make -j$(nproc)

# Set up entrypoint
ENTRYPOINT ["/app/build/logrescuer"]

# Default command shows help
CMD ["--help"]
