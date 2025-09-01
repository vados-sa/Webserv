#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html")
print()

print("<html><body>")
print("<h1>CGI Environment Variables</h1>")
print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")

for key, value in sorted(os.environ.items()):
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")

print("</table>")
print("<h2>Working Directory:</h2>")
print(f"<p>{os.getcwd()}</p>")
print("</body></html>")
