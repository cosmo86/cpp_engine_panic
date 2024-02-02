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
        programCache_data["RunningGroup"] = {}
else:
    programCache_data = {
        "LastCreated_SID": -1, 
        "Account info": {"username": "example_user", "balance": 1000},  # Example dictionary
        "PreviousRemoved_Strategy_Group":{}, # {int<s_id> , dict }
        "RunningGroup": {}, # {int<s_id> , dict }
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
mylib.startEngine.argtypes = [ctypes.c_char_p]
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
    running_mode = "test"
    running_mode=running_mode.encode('utf-8')
    mylib.startEngine(running_mode)

@app.on_event('shutdown')
def shutdown_event():
    mylib.stopEngine()
    with open(yaml_file_path, "w") as file:
        yaml.dump(programCache_data, file)
    print("Engine stopped, Strategy cached")

@app.get('/read_cached_data')
def read_cached_data():
    res = {"StrategyGroup":[]}
    for key, value in programCache_data["PreviousRemoved_Strategy_Group"].items():
        res["StrategyGroup"].append(value)
    return res

@app.post('/add_strategy')
def add_strategy(user_input: UserStrategyModel):
    if len(user_input.SecurityID) != 6:
        raise  HTTPException(status_code=400, detail="Bad Request: [Wrong SecurityID format]")
    if user_input.SecurityID.startswith('6') and user_input.ExchangeID == '2':#SZSE
        raise  HTTPException(status_code=400, detail="Bad Request: [Wrong ExchangeID]")
    if (user_input.SecurityID.startswith('0') or user_input.SecurityID.startswith('3')) and user_input.ExchangeID == '1':#SSE
        raise  HTTPException(status_code=400, detail="Bad Request: [Wrong ExchangeID]")
    # Increament SID by 1 and assign it to user_input
    programCache_data["LastCreated_SID"] += 1
    user_input.ID = str(programCache_data["LastCreated_SID"])
    json_str = user_input.model_dump_json()
    mylib.AddStrategy(json_str.encode('utf-8'))
    print(f"type of user_input is {type(user_input)}")
    programCache_data["PreviousRemoved_Strategy_Group"][int(user_input.ID)] = user_input.model_dump()
    programCache_data["RunningGroup"][int(user_input.ID)] = user_input.model_dump()

@app.post('/remove_strategy')
def remove_strategy(user_input: UserStrategyModel):
    ID_toRemove = int(user_input.ID)
    SecurityID = programCache_data["RunningGroup"][ID_toRemove]["SecurityID"]
    ExchangeID = programCache_data["RunningGroup"][ID_toRemove]["ExchangeID"]
    mylib.RemoveStrategy(ID_toRemove, SecurityID.encode('utf-8'), ExchangeID[0].encode('utf-8'))
    try:
        del programCache_data["RunningGroup"][ID_toRemove]
    except:
        print("Wrong ID, current IDs are: ", list(programCache_data["RunningGroup"].keys()) )

@app.post('/update_strategy_delay_time')
def update_strategy_delay_time(user_input: UserStrategyModel):
    mylib.UpdateDelayDuration(int(user_input.ID),user_input.DelayTime)

@app.get('/check_running_strategy')
def check_running_strategy():
    res = mylib.CheckRunningStrategy()
    print(f"res is {res}, type is {type(res)}, res.decode() is {res.decode('utf-8')}, size is {len(res.decode('utf-8'))}type is {type(res.decode('utf-8'))}")
    if len(res.decode('utf-8'))>5:
        rtn = {"StrategyGroup":[]}
        json_dict = json.loads(res)
        for dict in json_dict:
            model_instance = UserStrategyModel(**dict)
            rtn["StrategyGroup"].append(model_instance)
        return rtn
    else:
        print("No running strategy")
        return {"StrategyGroup":[UserStrategyModel()]}


@app.get('/check_removed_strategy')
def check_removed_strategy():
    res = mylib.CheckRemovedStrategy()
    print(f"res is {res}, type is {type(res)}, res.decode() is {res.decode('utf-8')}, size is {len(res.decode('utf-8'))}type is {type(res.decode('utf-8'))}")
    if len(res.decode('utf-8'))>50:
        rtn = {"StrategyGroup":[]}
        for key, value in programCache_data["PreviousRemoved_Strategy_Group"].items():
            if key not in programCache_data["RunningGroup"]:
                rtn["StrategyGroup"].append(value)

        json_dict = json.loads(res)
        for dict in json_dict:
            model_instance = UserStrategyModel(**dict)
            rtn["StrategyGroup"][int(model_instance.ID)] = model_instance

        return rtn
    else:
        print("No removed strategy")
        rtn = {"StrategyGroup":[UserStrategyModel()]}
        for key, value in programCache_data["PreviousRemoved_Strategy_Group"].items():
            if key not in programCache_data["RunningGroup"]:
                rtn["StrategyGroup"].append(value)
        return rtn



@app.get('/get_event_q_size')
def get_event_q_size():
    res = mylib.GetEventQSize()
    print("event q size ", res)

if __name__ == "__main__":
    uvicorn.run(app,port = 9001)