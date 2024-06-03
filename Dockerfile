# Use a base image with necessary dependencies
FROM ubuntu:latest

# Update package lists and install required dependencies
RUN apt-get update && \
    apt-get install -y \
    software-properties-common && \
    add-apt-repository universe && \
    apt-get update && \
    apt-get install -y \
    cmake \
    make \
    build-essential \
    g++ \
    gcc \
    git \
    libgtk-3-dev \
    libpcap-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the schunkdiscover source code into the container
COPY . /schunkdiscover


# Set the working directory
WORKDIR /schunkdiscover

# Create the build directory
RUN mkdir build

# Perform an out-of-source build
RUN cd build && \
    cmake .. && \
    make
    

# Expose any necessary ports if applicable
RUN cd build && \
  cmake -DBUILD_SCHUNKDISCOVER_GUI=OFF -DBUILD_SCHUNKDISCOVER_SHARED_LIB=OFF -DCMAKE_EXE_LINKER_FLAGS=\"-static\" .. && make install



# Set the entry point or command to execute the application every 30 seconds
WORKDIR /schunkdiscover/build/tools
ENTRYPOINT /bin/bash
# CMD ["sh", "-c", "while :; do ./build/tools/schunkdiscover; sleep 30; done"]


