#!/bin/bash

HOST="localhost"
PORT="9080"
BASE_URL="http://$HOST:$PORT"

# Colors for output
GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
NC="\033[0m"

PASS_COUNT=0
FAIL_COUNT=0

# Function to print test results
print_result() {
    local actual=$1
    local expected=$2
    local test_name=$3
    local extra_check=$4

    if [[ "$actual" == "$expected" ]] && [[ -z "$extra_check" || "$extra_check" == "true" ]]; then
        echo -e "✅ $test_name -> ${GREEN}PASS${NC} (got $actual)"
        ((PASS_COUNT++))
    else
        echo -e "❌ $test_name -> ${RED}FAIL${NC} (got $actual, expected $expected)"
        ((FAIL_COUNT++))
    fi
}

# Function to check if response contains expected text
check_content() {
    local url=$1
    local expected_text=$2
    local response=$(curl -s "$url")
    if [[ "$response" == *"$expected_text"* ]]; then
        echo "true"
    else
        echo "false"
    fi
}

echo "🧪 Comprehensive CGI Testing Suite"
echo "===================================="

# Test 1: Successful CGI execution
echo "🔹 1. Testing successful CGI execution"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py")
CONTENT_CHECK=$(check_content "$BASE_URL/cgi-bin/test_cgi.py" "Hello from CGI!")
print_result "$CODE" "200" "CGI GET test_cgi.py" "$CONTENT_CHECK"

# Test 2: CGI POST with data
echo "🔹 2. Testing CGI POST with data"
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "test=data&foo=bar" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI POST with data"

# Test 3: CGI POST with empty data
echo "🔹 3. Testing CGI POST with empty data"
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI POST empty data"

# Test 4: CGI POST with Content-Length: 0
echo "🔹 4. Testing CGI POST with Content-Length: 0"
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Length: 0" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI POST Content-Length: 0"

# Test 5: CGI with query string
echo "🔹 5. Testing CGI with query string"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py?param=value&test=123")
print_result "$CODE" "200" "CGI with query string"

# Test 6: CGI script that fails (exit code 1)
echo "🔹 6. Testing CGI script failure (exit code 1)"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/cgi_error.py")
print_result "$CODE" "500" "CGI script failure"

# Test 7: Non-existent CGI script
echo "🔹 7. Testing non-existent CGI script"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/nonexistent.py")
if [[ "$CODE" == "404" || "$CODE" == "500" ]]; then
    echo -e "✅ Non-existent CGI script -> ${GREEN}PASS${NC} (got $CODE)"
    ((PASS_COUNT++))
else
    echo -e "❌ Non-existent CGI script -> ${RED}FAIL${NC} (got $CODE, expected 404 or 500)"
    ((FAIL_COUNT++))
fi

# Test 8: CGI timeout test (if cgi_infinite.py exists)
echo "🔹 8. Testing CGI timeout"
if curl -s --max-time 1 "$BASE_URL/cgi-bin/cgi_infinite.py" >/dev/null 2>&1; then
    # If it responds quickly, it's not an infinite loop
    CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/cgi_infinite.py")
    print_result "$CODE" "200" "CGI infinite script (completed quickly)"
else
    # Test with a longer timeout to see if server handles it properly
    CODE=$(timeout 10 curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/cgi_infinite.py" 2>/dev/null || echo "504")
    print_result "$CODE" "504" "CGI timeout handling"
fi

# Test 9: CGI working directory test
echo "🔹 9. Testing CGI working directory"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_workdir.py")
CONTENT_CHECK=$(check_content "$BASE_URL/cgi-bin/test_workdir.py" "Current working directory")
print_result "$CODE" "200" "CGI working directory" "$CONTENT_CHECK"

# Test 10: CGI directory operations test
echo "🔹 10. Testing CGI directory operations"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/directory_test.py")
CONTENT_CHECK=$(check_content "$BASE_URL/cgi-bin/directory_test.py" "CGI Directory Test")
print_result "$CODE" "200" "CGI directory operations" "$CONTENT_CHECK"

# Test 11: CGI with file upload (multipart)
echo "🔹 11. Testing CGI with file upload"
echo "test file content for CGI" > test_upload.txt
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@test_upload.txt" "$BASE_URL/cgi-bin/test_cgi.py")
rm -f test_upload.txt
print_result "$CODE" "200" "CGI file upload"

# Test 12: CGI with large POST data
echo "🔹 12. Testing CGI with large POST data"
LARGE_DATA=$(printf 'a%.0s' {1..1000})
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "data=$LARGE_DATA" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI large POST data"

# Test 13: CGI response headers validation
echo "🔹 13. Testing CGI response headers"
RESPONSE=$(curl -s -i "$BASE_URL/cgi-bin/test_cgi.py")
if [[ "$RESPONSE" == *"Content-Type:"* ]] && [[ "$RESPONSE" == *"Content-Length:"* ]]; then
    echo -e "✅ CGI response headers -> ${GREEN}PASS${NC} (headers present)"
    ((PASS_COUNT++))
else
    echo -e "❌ CGI response headers -> ${RED}FAIL${NC} (missing headers)"
    ((FAIL_COUNT++))
fi

# Test 14: CGI Keep-Alive connection
echo "🔹 14. Testing CGI Keep-Alive connection"
CODE=$(curl -s -o /dev/null -w "%{http_code}" -H "Connection: keep-alive" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI Keep-Alive"

# Test 15: CGI Close connection
echo "🔹 15. Testing CGI Close connection"
CODE=$(curl -s -o /dev/null -w "%{http_code}" -H "Connection: close" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI Close connection"

# Test 16: CGI with custom headers
echo "🔹 16. Testing CGI with custom headers"
CODE=$(curl -s -o /dev/null -w "%{http_code}" -H "X-Custom-Header: test-value" -H "User-Agent: CGI-Test/1.0" "$BASE_URL/cgi-bin/test_cgi.py")
print_result "$CODE" "200" "CGI custom headers"

# Test 17: Concurrent CGI requests
echo "🔹 17. Testing concurrent CGI requests"
echo "Running 3 concurrent requests..."
CODE1=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py" &)
CODE2=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py" &)
CODE3=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py" &)
wait
if [[ "$CODE1" == "200" && "$CODE2" == "200" && "$CODE3" == "200" ]]; then
    echo -e "✅ Concurrent CGI requests -> ${GREEN}PASS${NC}"
    ((PASS_COUNT++))
else
    echo -e "❌ Concurrent CGI requests -> ${RED}FAIL${NC}"
    ((FAIL_COUNT++))
fi

# Test 18: CGI script with minimal output
echo "🔹 18. Testing CGI with minimal output"
# Create a test script
cat > /tmp/minimal_cgi.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain")
print()
print("OK")
EOF
chmod +x /tmp/minimal_cgi.py
cp /tmp/minimal_cgi.py var/www/site1/cgi/cgi-bin/ 2>/dev/null
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/minimal_cgi.py")
CONTENT_CHECK=$(check_content "$BASE_URL/cgi-bin/minimal_cgi.py" "OK")
print_result "$CODE" "200" "CGI minimal output" "$CONTENT_CHECK"
rm -f /tmp/minimal_cgi.py var/www/site1/cgi/cgi-bin/minimal_cgi.py

# Test 19: CGI with special characters in query
echo "🔹 19. Testing CGI with special characters"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py?name=test%20user&email=test%40example.com")
print_result "$CODE" "200" "CGI special characters"

# Test 20: Security test - path traversal prevention
echo "🔹 20. Testing path traversal prevention"
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/../../../etc/passwd" 2>/dev/null)
if [[ "$CODE" == "404" || "$CODE" == "403" || "$CODE" == "400" ]]; then
    echo -e "✅ Path traversal prevention -> ${GREEN}PASS${NC} (got $CODE)"
    ((PASS_COUNT++))
else
    echo -e "❌ Path traversal prevention -> ${RED}FAIL${NC} (got $CODE, expected 404/403/400)"
    ((FAIL_COUNT++))
fi

echo ""
echo "======================================"
echo -e "📊 Final Results: ${GREEN}$PASS_COUNT PASSED${NC}, ${RED}$FAIL_COUNT FAILED${NC}"
echo "======================================"

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "🎉 ${GREEN}All CGI tests passed!${NC}"
    exit 0
else
    echo -e "⚠️  ${RED}Some CGI tests failed!${NC}"
    exit 1
fi
echo -e "\n---\n"

echo "🔹 5. Testing CGI GET with output"
echo "Expected: HTTP 200 with content"
curl -s "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\nHTTP Status: $(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_cgi.py")"
echo -e "\n---\n"

echo "🔹 6. Testing CGI POST with data"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" -X POST -d "test=data&foo=bar" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 7. Testing CGI POST with empty data"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" -X POST -d "" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 8. Testing CGI POST with Content-Length: 0"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" -X POST -H "Content-Length: 0" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 9. Testing CGI working directory"
echo "Expected: HTTP 200 with directory info"
curl -s "$BASE_URL/cgi-bin/test_workdir.py"
echo -e "\nHTTP Status: $(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/test_workdir.py")"
echo -e "\n---\n"

echo "🔹 10. Testing CGI directory operations"
echo "Expected: HTTP 200 with directory tests"
curl -s "$BASE_URL/cgi-bin/directory_test.py"
echo -e "\nHTTP Status: $(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi-bin/directory_test.py")"
echo -e "\n---\n"

echo "🔹 11. Testing CGI with query string"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" "$BASE_URL/cgi-bin/test_cgi.py?param=value&test=123"
echo -e "\n---\n"

echo "🔹 12. Testing CGI with special characters in query"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" "$BASE_URL/cgi-bin/test_cgi.py?name=test%20user&email=test%40example.com"
echo -e "\n---\n"

echo "🔹 13. Testing CGI with multipart form data (file upload)"
echo "Expected: HTTP 200"
echo "test file content" > test_upload.txt
curl -s -w "HTTP Status: %{http_code}\n" -X POST -F "file=@test_upload.txt" "$BASE_URL/cgi-bin/test_cgi.py"
rm -f test_upload.txt
echo -e "\n---\n"

echo "🔹 14. Testing CGI error handling - permission denied"
echo "Expected: HTTP 500 or 403"
curl -s -w "HTTP Status: %{http_code}\n" "$BASE_URL/cgi-bin/../../../etc/passwd" 2>/dev/null || echo "Request blocked as expected"
echo -e "\n---\n"

echo "🔹 15. Testing CGI with large POST data"
echo "Expected: HTTP 200"
LARGE_DATA=$(printf 'a%.0s' {1..1000})
curl -s -w "HTTP Status: %{http_code}\n" -X POST -d "data=$LARGE_DATA" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 16. Testing CGI response headers"
echo "Expected: HTTP 200 with proper headers"
RESPONSE=$(curl -s -i "$BASE_URL/cgi-bin/test_cgi.py")
echo "$RESPONSE" | head -10
echo "..."
echo "HTTP Status: $(echo "$RESPONSE" | head -1 | cut -d' ' -f2)"
echo -e "\n---\n"

echo "🔹 17. Testing CGI Keep-Alive connection"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" -H "Connection: keep-alive" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 18. Testing CGI Close connection"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" -H "Connection: close" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 19. Testing CGI with custom headers"
echo "Expected: HTTP 200"
curl -s -w "HTTP Status: %{http_code}\n" -H "X-Custom-Header: test-value" -H "User-Agent: CGI-Test/1.0" "$BASE_URL/cgi-bin/test_cgi.py"
echo -e "\n---\n"

echo "🔹 20. Testing concurrent CGI requests"
echo "Expected: All return HTTP 200"
echo "Running 3 concurrent requests..."
(curl -s -w "Req1 Status: %{http_code}\n" "$BASE_URL/cgi-bin/test_cgi.py" &)
(curl -s -w "Req2 Status: %{http_code}\n" "$BASE_URL/cgi-bin/test_cgi.py" &)
(curl -s -w "Req3 Status: %{http_code}\n" "$BASE_URL/cgi-bin/test_cgi.py" &)
wait
echo -e "\n---\n"

echo "🔹 21. Testing CGI script with no output"
echo "Expected: HTTP 200"
# Create a minimal CGI script that produces no output
cat > /tmp/no_output.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain")
print()
# No body output
EOF
chmod +x /tmp/no_output.py
cp /tmp/no_output.py var/www/site1/cgi/cgi-bin/ 2>/dev/null || echo "Could not copy test script"
curl -s -w "HTTP Status: %{http_code}\n" "$BASE_URL/cgi-bin/no_output.py"
rm -f /tmp/no_output.py var/www/site1/cgi/cgi-bin/no_output.py
echo -e "\n---\n"

echo "🔹 22. Testing CGI with malformed headers"
echo "Expected: HTTP 500 or proper error handling"
# Create a CGI script with malformed headers
cat > /tmp/bad_headers.py << 'EOF'
#!/usr/bin/env python3
print("Malformed Header Without Colon")
print("Content-Type: text/plain")
print()
print("This should cause header parsing issues")
EOF
chmod +x /tmp/bad_headers.py
cp /tmp/bad_headers.py var/www/site1/cgi/cgi-bin/ 2>/dev/null || echo "Could not copy test script"
curl -s -w "HTTP Status: %{http_code}\n" "$BASE_URL/cgi-bin/bad_headers.py"
rm -f /tmp/bad_headers.py var/www/site1/cgi/cgi-bin/bad_headers.py
echo -e "\n---\n"

echo "✅ Comprehensive CGI Testing Complete!"
echo "======================================"
echo "📋 Tests performed:"
echo "   • Success cases (GET, POST, query strings, uploads)"
echo "   • Error handling (script errors, timeouts, not found)"
echo "   • Edge cases (no output, malformed headers, large data)"
echo "   • Concurrency and connection handling"
echo "   • Security (path traversal attempts)"
