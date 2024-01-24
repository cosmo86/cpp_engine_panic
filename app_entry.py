import ctypes
import os
import yaml
from datetime import datetime
from fastapi_model.dataModels import UserStrategyModel,UserStrategyGroupModel
import json
import uvicorn
from fastapi import FastAPI, HTTPException

############### Cache Initialization Start ###############
# Create the cache folder 
folder_name = "program_cache"
current_date_str = datetime.now().strftime('%Y_%m_%d')
if not os.path.exists(folder_name):
    os.makedirs(folder_name)
# Define the YAML file path
yaml_file_path = os.path.join(folder_name, f"SE_{current_date_str}.yaml")
if os.path.exists(yaml_file_path):
    with open(yaml_file_path, "r") as file:
        programCache_data = yaml.safe_load(file)
else:
    programCache_data = {
        "LastCreated_SID": -1, 
        "Account info": {"username": "example_user", "balance": 1000},  # Example dictionary
        "PreviousRunning_Strategy_Group": UserStrategyGroupModel().model_dump(),
        "PreviousRemoved_Strategy_Group":UserStrategyGroupModel().model_dump(),
        "RecordTime" : datetime.now().strftime('%Y_%m_%d_%H:%M:%S'),
    }

############### Cache Initialization Done ###############

############### Linked Library Initialization Start ###############
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
############### Linked Library Initialization Done ###############

###############
# Start FastApi
app = FastAPI()
print(os.getcwd())


@app.on_event('startup')
def startup_event():
    print("[fastAPI] start engine")
    mylib.initEngine()
    mylib.startEngine()

@app.on_event('shutdown')
def shutdown_event():
    mylib.stopEngine()
    with open(yaml_file_path, "w") as file:
        yaml.dump(programCache_data, file)
    print("Engine stopped, Strategy cached")

@app.get('/read_cached_data')
def read_cached_data():
    return programCache_data

@app.post('/add_strategy')
def add_strategy(user_input: UserStrategyModel):
    # Increament SID by 1 and assign it to user_input
    programCache_data["LastCreated_SID"] += 1
    user_input.ID = programCache_data["LastCreated_SID"]
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