#!/usr/bin/env python3
import os, sys
print("Content-Type: text/plain\n")
print("Hello from CGI!")
print("Method:", os.environ.get("REQUEST_METHOD"))
if os.environ.get("REQUEST_METHOD") == "POST":
    print("POST data:", sys.stdin.read())
print("CWD:", os.getcwd())