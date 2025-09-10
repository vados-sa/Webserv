#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import urllib.parse
import html
import http.cookies

def parse_query():
    """Parse query string from GET."""
    return urllib.parse.parse_qs(os.environ.get("QUERY_STRING", ""))

def parse_post():
    """Parse body from POST (x-www-form-urlencoded)."""
    if os.environ.get("REQUEST_METHOD", "") != "POST":
        return {}
    try:
        length = int(os.environ.get("CONTENT_LENGTH", 0))
    except ValueError:
        length = 0
    body = sys.stdin.read(length)
    return urllib.parse.parse_qs(body)

def get_cookies():
    """Parse cookies from HTTP_COOKIE."""
    cookie_header = os.environ.get("HTTP_COOKIE", "")
    cookies = http.cookies.SimpleCookie()
    cookies.load(cookie_header)
    return {key: morsel.value for key, morsel in cookies.items()}

def main():
    try:
        method = os.environ.get("REQUEST_METHOD", "GET")
        query = parse_query()
        post = parse_post()
        cookies = get_cookies()

        name = (
            query.get("name", [None])[0]
            or post.get("name", [None])[0]
            or cookies.get("name")
            or "stranger"
        )

        # Prepare new cookie
        cookie = http.cookies.SimpleCookie()
        cookie["name"] = name
        cookie["name"]["path"] = "/"
        cookie["name"]["max-age"] = 3600  # 1 hour

        # Send headers
        print("Content-Type: text/html")
        print(cookie.output())
        print()  # End of headers

        # HTML response
        print("<!DOCTYPE html>")
        print("<html><head><title>CGI Demo</title></head><body>")
        print(f"<h1>Hello, {html.escape(name)}!</h1>")
        print("<p>This page demonstrates a robust Python CGI script.</p>")

        print("<h2>Request Details</h2>")
        print("<ul>")
        print(f"<li>Method: {html.escape(method)}</li>")
        print(f"<li>Query params: {html.escape(str(query))}</li>")
        print(f"<li>POST params: {html.escape(str(post))}</li>")
        print(f"<li>Cookies: {html.escape(str(cookies))}</li>")
        print("</ul>")

        print("<h2>Try it</h2>")
        print('<form method="post">')
        print('  <label>Your name: <input type="text" name="name"></label>')
        print('  <input type="submit" value="Submit via POST">')
        print("</form>")

        print('<form method="get">')
        print('  <label>Your name: <input type="text" name="name"></label>')
        print('  <input type="submit" value="Submit via GET">')
        print("</form>")

        print("</body></html>")

    except Exception as e:
        # Fallback for unexpected errors
        print("Status: 500 Internal Server Error")
        print("Content-Type: text/plain\n")
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
