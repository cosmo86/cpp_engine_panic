import ctypes
import os
import time
from fastapi_model.dataModels import UserStrategyModel,UserStrategyGroupModel
import json
import uvicorn
from fastapi import FastAPI, HTTPException

app = FastAPI()
# Print the current working directory
print(os.getcwd())

mylib = ctypes.CDLL('build/lib/libMyLibrary.so')

# Configure return types
mylib.testreturnstr.restype = ctypes.c_char_p
mylib.testreturnint.restype = ctypes.c_int
mylib.TestRtnJsonStr.restype = ctypes.c_char_p
mylib.GetEventQSize = ctypes.c_int
mylib.CheckRunningStrategy.restype = ctypes.c_char_p
mylib.CheckRemovedStrategy.restype = ctypes.c_char_p

# Configure argument types
mylib.testtakestr.argtypes = [ctypes.c_char_p]
mylib.TestRtnJsonStr.argtypes = [ctypes.c_char_p]
mylib.AddStrategy.argtypes = [ctypes.c_char_p]
mylib.RemoveStrategy.argtypes = [ctypes.c_int, ctypes.c_char_p,ctypes.c_char]
mylib.UpdateDelayDuration.argtypes = [ctypes.c_int, ctypes.c_int]

@app.on_event('startup')
def startup_event():
    print("[fastAPI] start engine")
    mylib.initEngine()
    mylib.startEngine()

@app.on_event('shutdown')
def shutdown_event():
    mylib.stopEngine()
    print("Engine stopped")

@app.post('/add_strategy')
def add_strategy(user_input: UserStrategyModel):
    json_str = user_input.model_dump_json()
    mylib.AddStrategy(json_str.encode('utf-8'))

@app.post('/remove_strategy')
def remove_strategy(user_input: UserStrategyModel):
    mylib.RemoveStrategy(int(user_input.ID), user_input.SecurityID.encode('utf-8'),user_input.ExchangeID[0].encode('utf-8'))

@app.post('/update_strategy_delay_time')
def update_strategy_delay_time(user_input: UserStrategyModel):
    mylib.UpdateDelayDuration(int(user_input.ID),user_input.DelayTime)


@app.get('/check_running_strategy')
def check_running_strategy():
    res = mylib.CheckRunningStrategy()
    print(f"res is {res}, type is {type(res)}, res.decode() is {res.decode('utf-8')}, size is {len(res.decode('utf-8'))}type is {type(res.decode('utf-8'))}")
    if len(res.decode('utf-8'))>5:
        strategy_group = UserStrategyGroupModel()
        json_dict = json.loads(res)
        for dict in json_dict:
            model_instance = UserStrategyModel(**dict)
            strategy_group.StrategyGroup.append(model_instance)
        return strategy_group
    else:
        print("No running strategy")
        raise HTTPException(status_code=400, detail="Empty: [No running strategy]")


@app.get('/check_removed_strategy')
def check_removed_strategy():
    res = mylib.CheckRemovedStrategy()
    print(f"res is {res}, type is {type(res)}, res.decode() is {res.decode('utf-8')}, size is {len(res.decode('utf-8'))}type is {type(res.decode('utf-8'))}")
    if len(res.decode('utf-8'))>50:
        strategy_group = UserStrategyGroupModel()
        json_dict = json.loads(res)
        for dict in json_dict:
            model_instance = UserStrategyModel(**dict)
            strategy_group.StrategyGroup.append(model_instance)
        return strategy_group
    else:
        print("No removed strategy")
        raise HTTPException(status_code=400, detail="Empty: [No running strategy]")

@app.get('/get_event_q_size')
def get_event_q_size():
    res = mylib.GetEventQSize()
    print("event q size ", res)






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


if __name__ == "__main__":
    uvicorn.run(app,port = 9000)