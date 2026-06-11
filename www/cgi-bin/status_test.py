#!/usr/bin/env python3
import os
import sys

# We will read the desired status code from the Query String!
# Example: ?code=404
query = os.environ.get("QUERY_STRING", "")
status_code = "200 OK" # Default

if "code=404" in query:
    status_code = "404 Not Found"
elif "code=418" in query:
    status_code = "418 I'm a teapot"
elif "code=500" in query:
    status_code = "500 Internal Server Error"

# 1. Print the custom Status header
print(f"Status: {status_code}\r")

# 2. Print the mandatory Content-Type
print("Content-Type: text/plain\r\n\r")

# 3. Print the body
print(f"The CGI script requested status: {status_code}")