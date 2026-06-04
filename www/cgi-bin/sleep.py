#!/usr/bin/env python3
import time

print("Content-Type: text/plain\r\n\r\n", end="")
time.sleep(10) # Simulate a hanging script
print("If you see this, your timeout failed.")