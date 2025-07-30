#!/bin/bash

HOST="localhost"
PORT="8080"
BASE_URL="http://$HOST:$PORT"
UPLOAD_DIR="upload"
TEST_FILE="test_upload.txt"
UNKNOWN_METHOD="FOO"

echo "🧪 Creating test file..."
echo "Hello Webserv!" > $TEST_FILE

echo "🔹 1. GET /"
curl -i "$BASE_URL/" || echo "❌ GET / failed"
echo -e "\n---\n"

echo "🔹 2. POST upload"
curl -i -X POST -F "file=@$TEST_FILE" "$BASE_URL/$UPLOAD_DIR/" || echo "❌ POST upload failed"
echo -e "\n---\n"

echo "🔹 3. GET uploaded file"
curl -i "$BASE_URL/$UPLOAD_DIR/$TEST_FILE" || echo "❌ GET uploaded file failed"
echo -e "\n---\n"

echo "🔹 4. DELETE uploaded file"
curl -i -X DELETE "$BASE_URL/$UPLOAD_DIR/$TEST_FILE" || echo "❌ DELETE failed"
echo -e "\n---\n"

echo "🔹 5. Confirm deletion (should return 404)"
curl -i "$BASE_URL/$UPLOAD_DIR/$TEST_FILE" || echo "❌ GET after delete failed"
echo -e "\n---\n"

echo "🔹 6. UNKNOWN method (FOO)"
curl -i -X $UNKNOWN_METHOD "$BASE_URL/" || echo "❌ UNKNOWN method test failed"
echo -e "\n---\n"

echo "✅ Done!"
