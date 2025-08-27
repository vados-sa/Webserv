#!/usr/bin/env python3

import os
import urllib.parse
import sys

request_method = os.environ.get("REQUEST_METHOD", "GET")
# Get query string (works for both GET and POST)
query_string = os.environ.get("QUERY_STRING", "")
query_params = urllib.parse.parse_qs(query_string)
path_info = os.environ.get("SCRIPT_FILENAME", "")

# Handle POST data
post_data = ""
if request_method == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    post_data = sys.stdin.read(content_length)
    post_params = urllib.parse.parse_qs(post_data)
else:
    post_params = {}

print(f"Content-Type: text/html", end="\r\n")
print("", end="\r\n")  # Body separator

# Debug output
print(f"<p><strong>Request Method:</strong> {request_method}</p>")
print(f"<p><strong>Query String:</strong> {query_string}</p>")
print(f"<p><strong>Query Parameters:</strong> {query_params}</p>")
print(f"<p><strong>POST Data:</strong> {post_data}</p>")
print(f"<p><strong>POST Parameters:</strong> {post_params}</p>")
