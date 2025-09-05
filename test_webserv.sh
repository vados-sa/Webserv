#!/bin/bash
# Webserver Test Script (compatible with bash 3.x)
# Usage: ./test_webserv.sh [host] [port]
# Example: ./test_webserv.sh 127.0.0.1 9080

HOST=${1:-127.0.0.1}
PORT=${2:-8080}
BASE_URL="http://$HOST:$PORT"

# Colors
GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
NC="\033[0m"

PASS_COUNT=0
FAIL_COUNT=0

# Function: allowed methods per location
get_allowed_methods() {
    local location="$1"
    case "$location" in
        "/") echo "GET DELETE" ;;
        "/upload") echo "GET POST DELETE" ;;       # DELETE not allowed
        "/images") echo "GET POST DELETE" ;;
        "/redirect-me") echo "GET" ;;
        "/cgi-bin") echo "GET POST" ;;
        "/php") echo "GET POST" ;;
        *) echo "GET" ;;
    esac
}

# Function: expected status code for method at location
get_expected() {
    local location="$1"
    local method="$2"
    local allowed
    allowed=$(get_allowed_methods "$location")
    if [[ " $allowed " =~ " $method " ]]; then
        if [[ "$method" == "POST" ]]; then
            echo 200
        else
            echo 200
        fi
    else
        echo 405
    fi
}

# Print test result
print_result() {
    local code=$1
    local expected=$2
    local testname=$3

    if [[ "$code" == "$expected" ]]; then
        echo -e "✅ $testname -> ${GREEN}PASS${NC} (got $code)"
        ((PASS_COUNT++))
    else
        echo -e "❌ $testname -> ${RED}FAIL${NC} (got $code, expected $expected)"
        ((FAIL_COUNT++))
    fi
}

echo "🔍 Testing webserver at $BASE_URL"
echo "------------------------------------"

# --------------------
# GET /
CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/)
EXPECTED=$(get_expected / GET)
print_result $CODE $EXPECTED "GET /"

# --------------------
# POST /
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST $BASE_URL/ -d "test=data")
EXPECTED=$(get_expected / POST)
print_result $CODE $EXPECTED "POST /"

# --------------------
# POST /upload (file upload)
echo "upload test content" > upload_test.txt
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST $BASE_URL/upload -F "file=@upload_test.txt")
EXPECTED=$(get_expected /upload POST)
if [[ "$CODE" == "$EXPECTED" || "$CODE" == "201" ]]; then
    curl -s $BASE_URL/upload/upload_test.txt -o download_test.txt
    if diff upload_test.txt download_test.txt > /dev/null; then
        echo -e "✅ POST & GET /upload file -> ${GREEN}PASS${NC}"
        ((PASS_COUNT++))
    else
        echo -e "❌ POST & GET /upload file -> ${RED}FAIL${NC} (content mismatch)"
        ((FAIL_COUNT++))
    fi
else
    echo -e "❌ POST /upload -> ${RED}FAIL${NC} (got $CODE, expected $EXPECTED)"
    ((FAIL_COUNT++))
fi

# --------------------
# DELETE /upload/file
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE $BASE_URL/upload/upload_test.txt)
EXPECTED=$(get_expected /upload DELETE)
print_result $CODE 204 "DELETE /upload/upload_test.txt"

# --------------------
# UNKNOWN method
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X FOO $BASE_URL/)
print_result $CODE 405 "UNKNOWN method /"

# --------------------
# GET non-existing file
CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/doesnotexist.html)
print_result $CODE 404 "GET non-existing file"

# --------------------
# GET /autoindex/ (autoindex)
CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/autoindex/)
EXPECTED=$(get_expected /autoindex GET)
print_result $CODE $EXPECTED "GET /autoindex/ (autoindex)"

# --------------------
# GET /redirect-me
CODE=$(curl -s -o /dev/null -w "%{http_code}" -L $BASE_URL/redirect-me)
EXPECTED=$(get_expected /redirect-me GET)
print_result $CODE $EXPECTED "GET /redirect-me (redirect)"

# --------------------

# DELETE non-existing file
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE $BASE_URL/upload/doesnotexist.txt)
print_result $CODE 404 "DELETE /upload/doesnotexist.txt (non-existing file)"

# --------------------
# Upload & GET file (content match)
echo "hello upload world" > upload_check.txt
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST $BASE_URL/upload -F "file=@upload_check.txt")
if [[ "$CODE" == "200" || "$CODE" == "201" ]]; then
    curl -s $BASE_URL/upload/upload_check.txt -o download_check.txt
    if diff upload_check.txt download_check.txt > /dev/null; then
        echo -e "✅ Upload & GET file -> ${GREEN}PASS${NC}"
        ((PASS_COUNT++))
    else
        echo -e "❌ Upload & GET file -> ${RED}FAIL${NC} (content mismatch)"
        ((FAIL_COUNT++))
    fi
else
    echo -e "❌ Upload /upload -> ${RED}FAIL${NC} (got $CODE, expected 200/201)"
fi

# --------------------
# CGI TESTS

CGI_DIR="cgi-bin"
CGI_SCRIPT="test_cgi.py"
CGI_ERROR_SCRIPT="cgi_error.py"
CGI_LOOP_SCRIPT="cgi_infinite.py"

# --- CGI GET ---
CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/$CGI_DIR/$CGI_SCRIPT)
print_result $CODE 200 "CGI GET $CGI_SCRIPT"

# --- CGI POST ---
CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST -d "foo=bar&baz=qux" $BASE_URL/$CGI_DIR/$CGI_SCRIPT)
print_result $CODE 200 "CGI POST $CGI_SCRIPT"

# --- CGI relative path test (script should print CWD or access a relative file) ---
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/$CGI_DIR/$CGI_SCRIPT?file=relative.txt")
print_result $CODE 200 "CGI relative path $CGI_SCRIPT"

# --- CGI error script ---
CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/$CGI_DIR/$CGI_ERROR_SCRIPT)
if [[ "$CODE" == "500" || "$CODE" == "502" || "$CODE" == "504" ]]; then
    echo -e "✅ CGI error script -> ${GREEN}PASS${NC} (got $CODE)"
    ((PASS_COUNT++))
else
    echo -e "❌ CGI error script -> ${RED}FAIL${NC} (got $CODE, expected 500/502/504)"
    ((FAIL_COUNT++))
fi

# --- CGI infinite loop (should timeout, not crash server) ---
CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE_URL/$CGI_DIR/$CGI_LOOP_SCRIPT)
if [[ "$CODE" == "504" ]]; then
    echo -e "✅ CGI infinite loop timeout -> ${GREEN}PASS${NC} (got $CODE)"
    ((PASS_COUNT++))
else
    echo -e "❌ CGI infinite loop timeout -> ${RED}FAIL${NC} (got $CODE, expected 504)"
    ((FAIL_COUNT++))
fi

# Summary
echo
echo "------------------------------------"
echo -e "📊 Summary: ${GREEN}$PASS_COUNT PASS${NC}, ${RED}$FAIL_COUNT FAIL${NC}"
echo "------------------------------------"

# Exit with failure if any test failed
if [[ $FAIL_COUNT -ne 0 ]]; then
    exit 1
else
    exit 0
fi

rm *.txt