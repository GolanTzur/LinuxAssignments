#!/bin/bash

# Check Docker permissions
if ! docker info >/dev/null 2>&1; then
    echo "Error: Cannot connect to Docker daemon"
    echo "Either:"
    echo "1. Run this script with sudo: sudo $0"
    echo "2. Add your user to the docker group (recommended):"
    echo "   sudo usermod -aG docker $USER"
    echo "   (You'll need to log out and back in for this to take effect)"
    exit 1
fi

# Check if number of decrypters is provided as argument, default to 2 if not
NUM_DECRYPTERS=${1:-2}

# Define mount path - can be overridden by environment variable
MOUNT_PATH=${MOUNT_PATH:-"/mnt/mta"}
# Get the local directory name from the mount path
LOCAL_MOUNT_DIR=$(basename "$MOUNT_PATH")

# Validate input is a positive number
if ! [[ "$NUM_DECRYPTERS" =~ ^[0-9]+$ ]] || [ "$NUM_DECRYPTERS" -lt 1 ]; then
    echo "Error: Please provide a positive number for decrypters"
    echo "Usage: $0 [number_of_decrypters]"
    echo "Optional: Set MOUNT_PATH environment variable to change mount path (default: /mnt/mta)"
    exit 1
fi

# Clean up old logs and create required directories
rm -rf logs/*
mkdir -p "$LOCAL_MOUNT_DIR" logs/encrypter logs/decrypters
chmod -R 777 "$LOCAL_MOUNT_DIR" logs

# Create configuration file if it doesn't exist
if [ ! -f "$LOCAL_MOUNT_DIR/encrypter.conf" ]; then
    echo "PASSWORD_LENGTH=16" > "$LOCAL_MOUNT_DIR/encrypter.conf"
fi

# Clean up any existing containers
# Stop and remove encrypter container if it exists
docker rm -f encrypter_container 2>/dev/null || true
# Find and remove all decrypter containers
docker ps -a --format '{{.Names}}' | grep '^decrypter_[0-9]\+$' | xargs -r docker rm -f

# Create a temporary build directory
BUILD_DIR=$(mktemp -d)
trap 'rm -rf "$BUILD_DIR"' EXIT

# Copy only necessary files to build directory
cp encrypter.c mta-utils-dev-x86_64.deb decrypter.c common.h mta_crypt.h mta_rand.h Dockerfile.* "$BUILD_DIR/"

# Build images with mount path from the clean build directory
cd "$BUILD_DIR"
docker build -t encrypter -f Dockerfile.encrypter --build-arg MOUNT_PATH="$MOUNT_PATH" .
docker build -t decrypter -f Dockerfile.decrypter --build-arg MOUNT_PATH="$MOUNT_PATH" .
cd -

# Run encrypter
docker run -d --name encrypter_container \
    -v "$(pwd)/$LOCAL_MOUNT_DIR:$MOUNT_PATH" \
    -v "$(pwd)/logs/encrypter:/var/log" \
    encrypter >/dev/null

# Wait a bit for encrypter to initialize
sleep 2

# Run decrypters
for i in $(seq 1 $NUM_DECRYPTERS); do
    mkdir -p "logs/decrypters/decrypter_$i"
    docker run -d --name "decrypter_$i" \
        -v "$(pwd)/$LOCAL_MOUNT_DIR:$MOUNT_PATH" \
        -v "$(pwd)/logs/decrypters/decrypter_$i:/var/log" \
        decrypter "$i" >/dev/null
done
