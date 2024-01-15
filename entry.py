import ctypes
import os
import time
# Print the current working directory
print(os.getcwd())



# Load the shared library
mylib = ctypes.CDLL('build/lib/libMyLibrary.so')

# Configure return types
mylib.testreturnstr.restype = ctypes.c_char_p
mylib.testreturnint.restype = ctypes.c_int

# Configure argument types
mylib.testtakestr.argtypes = [ctypes.c_char_p]

# Call the functions
str_result = mylib.testreturnstr().decode('utf-8')
int_result = mylib.testreturnint()
mylib.testtakestr(b"Hello from Python")

# Print the results
print("String from C++:", str_result)
print("Integer from C++:", int_result)

mylib.initEngine()
mylib.startEngine()

while True:
    time.sleep(1)