#!/usr/bin/env python3
# save as: cgi-bin/hello.py
# then run: chmod +x cgi-bin/hello.py

import os

print("Content-Type: text/html")
print("")  # this blank line is MANDATORY
print("<html><body><h1>CGI works!</h1></body></html>")