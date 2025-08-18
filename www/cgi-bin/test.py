#!/usr/bin/env python3
import os
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>CGI Test</h1>")
print(f"<p>Query String: {os.environ.get('QUERY_STRING')}</p>")
print(f"<p>Request Method: {os.environ.get('REQUEST_METHOD')}</p>")
print("</body></html>")