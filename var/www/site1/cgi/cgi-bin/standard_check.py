#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print()

print("=== Expected CGI Directory Behavior ===")
current_dir = os.getcwd()
script_dir = os.path.dirname(os.path.abspath(__file__))

print(f"Current working directory: {current_dir}")
print(f"Script directory: {script_dir}")
print()

if current_dir == script_dir:
    print("✅ CORRECT: CGI is running from script's directory")
    print("✅ Relative paths like './config.txt' will work as expected")
else:
    print("❌ INCORRECT: CGI is NOT running from script's directory")
    print("❌ This violates CGI standards")
    print("❌ Relative paths won't work as developers expect")

print()
print("=== Testing Relative File Access ===")

# Test if we can access files relative to where we SHOULD be running
try:
    # This should work if we're in the script directory
    with open("./test.py", "r") as f:
        print("✅ Can access './test.py' (relative to current directory)")
except:
    print("❌ Cannot access './test.py' from current directory")
    print("   (This means relative paths don't work as expected)")
