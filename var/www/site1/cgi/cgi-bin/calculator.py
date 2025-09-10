#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import cgi
import html

def main():
    print("Content-Type: text/html\r\n")
    print("<!DOCTYPE html>")
    print("<html><head><title>CGI Calculator</title></head><body>")
    print("<h1>Simple Calculator</h1>")

    form = cgi.FieldStorage()
    result = None

    try:
        a = form.getfirst("a")
        b = form.getfirst("b")
        op = form.getfirst("op")

        if a and b and op:
            a = float(a)
            b = float(b)
            if op == "add":
                result = a + b
            elif op == "sub":
                result = a - b
            elif op == "mul":
                result = a * b
            elif op == "div":
                result = a / b if b != 0 else "Error: divide by zero"
            else:
                result = "Invalid operation"
    except Exception as e:
        result = f"Error: {html.escape(str(e))}"

    # Form
    print("""
    <form method="get">
      <input type="text" name="a" placeholder="Number 1">
      <input type="text" name="b" placeholder="Number 2">
      <select name="op">
        <option value="add">+</option>
        <option value="sub">-</option>
        <option value="mul">*</option>
        <option value="div">/</option>
      </select>
      <input type="submit" value="Compute">
    </form>
    """)

    # Result
    if result is not None:
        print(f"<h2>Result: {html.escape(str(result))}</h2>")

    print("</body></html>")

if __name__ == "__main__":
    main()
