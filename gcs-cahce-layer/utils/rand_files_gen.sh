#!/bin/bash

# Number of files to generate
NUM_FILES=10

# Minimum and maximum size in MB
MIN_SIZE_MB=2
MAX_SIZE_MB=32

# Output directory
OUTPUT_DIR=$1

mkdir -p "$OUTPUT_DIR"

for i in $(seq 1 $NUM_FILES); do
    # Random size between MIN_SIZE_MB and MAX_SIZE_MB
    SIZE_MB=$((RANDOM % (MAX_SIZE_MB - MIN_SIZE_MB + 1) + MIN_SIZE_MB))
    FILE_NAME="$OUTPUT_DIR/random_file_${i}.bin"

    echo "Generating $FILE_NAME (${SIZE_MB}MB)..."
    head -c "$((SIZE_MB * 1024 * 1024))" /dev/urandom > "$FILE_NAME"
done

echo "All $NUM_FILES files (2–32MB) created in $OUTPUT_DIR"
