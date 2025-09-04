#!/bin/bash

SERVER="localhost:8080"

echo "1. Testing non-existent file (404)"
curl -i "$SERVER/nonexistent.txt"

echo -e "\n2. Testing directory without index and autoindex disabled (403)"
curl -i "$SERVER/dir_no_index"

echo -e "\n3. Testing directory with autoindex enabled (should return 200 with listing)"
curl -i "$SERVER/dir_with_autoindex"

echo -e "\n4. Testing file without read permission (403)"
curl -i "$SERVER/file_no_read.txt"

echo -e "\n5. Testing resource that is not a regular file (403)"
curl -i "$SERVER/special_device"

echo -e "\n6. Testing directory without permission (403)"
curl -i "$SERVER/dir_no_permission"

echo -e "\n7. Testing internal server error (500) - simulate stat error"
curl -i "$SERVER/internal_error"

# Note:
# - Adjust the paths according to your configuration.
# - To test permissions, create files/directories with appropriate chmod.
# - For "special_device", you can use a symlink, pipe, or device file.
