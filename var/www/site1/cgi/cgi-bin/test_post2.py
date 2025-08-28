#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/plain")
print()
print("REQUEST_METHOD:", os.environ.get("REQUEST_METHOD"))
print("CONTENT_LENGTH:", os.environ.get("CONTENT_LENGTH"))
print("BODY:")
body = sys.stdin.read(int(os.environ.get("CONTENT_LENGTH", 0)))
print(body)
print("BODY LENGTH:", len(body))
