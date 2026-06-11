#!/usr/bin/env python3
import os

print("Content-Type: text/plain\r\n\r")
print("Query String is: " + os.environ.get("QUERY_STRING", "Empty"))