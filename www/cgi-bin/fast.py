#!/usr/bin/env python3
import sys

# Read everything from stdin (POST body) and echo it back
body = sys.stdin.read()
print("Content-Type: text/plain\r\n\r\n", end="")
print(f"CGI Processed {len(body)} bytes successfully!")