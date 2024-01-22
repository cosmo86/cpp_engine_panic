import ctypes
import os
import time
from fastapi_model.dataModels import UserStrategyModel
import json

# Print the current working directory
print(os.getcwd())

def call_cpp_function(model: UserStrategyModel) -> UserStrategyModel:
    # Convert the Pydantic model to a JSON string and call the C++ function
    json_str = model.model_dump_json()
    print("JSON string being sent to C++:", json_str)
    result_ptr = mylib.TestRtnJsonStr(json_str.encode('utf-8'))

    # Convert the returned C-style string to a Python string
    result_str = ctypes.c_char_p(result_ptr).value.decode('utf-8')

    # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    # Deallocate memory allocated in C++ (assuming you use 'new' in C++)
    #lib.deallocate_string(result_ptr)
    # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    model_dict = json.loads(result_str)

    # Iterate through the dictionary and print key-value pairs
    for key, value in model_dict.items():
        print(f"{key}: {value}")
    return

# Load the shared library
mylib = ctypes.CDLL('build/lib/libMyLibrary.so')

# Configure return types
mylib.testreturnstr.restype = ctypes.c_char_p
mylib.testreturnint.restype = ctypes.c_int
mylib.TestRtnJsonStr.argtypes = [ctypes.c_char_p]
mylib.TestRtnJsonStr.restype = ctypes.c_char_p

# Configure argument types
mylib.testtakestr.argtypes = [ctypes.c_char_p]

# Call the functions
str_result = mylib.testreturnstr().decode('utf-8')
int_result = mylib.testreturnint()
mylib.testtakestr(b"Hello from Python")


test_model = UserStrategyModel()
test_model.SecurityID = "600519"
test_model.ExchangeID = '2'
test_model.MaxTriggerTimes = 3
test_model.BuyTriggerVolume = 10000
test_model.CancelVolume = 20000
test_model.CancelVolume = Position = 100000
test_model.SecurityName = "贵州茅台"
test_model.ID = "12"
call_cpp_function(test_model)


# Print the results
print("String from C++:", str_result)
print("Integer from C++:", int_result)

#mylib.initEngine()
#mylib.startEngine()


while True:
    time.sleep(1)