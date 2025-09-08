#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain")
print()

try:
    # Try to create a test file in current directory
    with open("./cgi_test_file.txt", "w") as f:
        f.write("test")
    print("SUCCESS: Created test file in current directory")
    print(f"Current working directory: {os.getcwd()}")
    
    # Clean up
    os.remove("./cgi_test_file.txt")
except Exception as e:
    print(f"ERROR: {e}")
    print(f"Current working directory: {os.getcwd()}")
