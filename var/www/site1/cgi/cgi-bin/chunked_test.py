#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain")
print()

method = os.environ.get("REQUEST_METHOD", "")
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

print(f"Method: {method}")
print(f"Content-Length: {content_length}")

if method == "POST" and content_length > 0:
    body = sys.stdin.read(content_length)
    print(f"Body received: {body}")
    print(f"Body length: {len(body)}")
else:
    print("No POST body received")