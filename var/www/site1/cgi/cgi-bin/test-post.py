#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/html\r\n")
print()

print("<html><body><h1>Hello from Python CGI!</h1>")

if os.environ.get("REQUEST_METHOD", "") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))

    post_data = sys.stdin.read(content_length) if content_length > 0 else ""
    print("<h2>POST Data:</h2>")
    print(f"<pre>{post_data}</pre>")
else:
    print("<p>No POST data received.</p>")

print("</body></html>")