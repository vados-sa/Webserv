#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print()

print("=== CGI Directory Test ===")
print(f"Current working directory: {os.getcwd()}")
print(f"Script location (__file__): {__file__}")
print(f"Script directory: {os.path.dirname(os.path.abspath(__file__))}")
print()

# Test 1: Can we access files relative to the script?
script_dir = os.path.dirname(os.path.abspath(__file__))
test_file = os.path.join(script_dir, "test_cgi.py")
print(f"Looking for test_cgi.py in script directory: {test_file}")
print(f"File exists: {os.path.exists(test_file)}")
print()

# Test 2: Try to create a file in current directory
try:
    with open("./cgi_created_file.txt", "w") as f:
        f.write("test")
    print("SUCCESS: Created file in current working directory")
    # Clean up
    os.remove("./cgi_created_file.txt")
except Exception as e:
    print(f"FAILED to create file in current directory: {e}")
print()

# Test 3: Try to access relative to script directory
try:
    relative_to_script = os.path.join(os.path.dirname(__file__), "test.py")
    print(f"Trying to access: {relative_to_script}")
    print(f"Relative script exists: {os.path.exists(relative_to_script)}")
except Exception as e:
    print(f"Error accessing relative to script: {e}")
