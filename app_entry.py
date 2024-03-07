import ctypes
import os
import signal
import yaml
from datetime import datetime
from fastapi_model.dataModels import UserStrategyModel,UserStrategyGroupModel
import json
import uvicorn
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

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

mylib.manual_Send_Order_LimitPrice.argtypes = [
    ctypes.c_char,       # exchange_id
    ctypes.c_int,        # volume
    ctypes.c_double,     # price
    ctypes.c_char_p,     # stock_id
    ctypes.c_char_p,     # req_sinfo
    ctypes.c_int,        # order_ref
    ctypes.c_int         # req_iinfo
]
mylib.manual_Send_Cancle_Order_OrderActionRef.argtypes = [
    ctypes.c_char,    # exchange_id
    ctypes.c_char_p,  # req_sinfo
    ctypes.c_int,     # order_ref
    ctypes.c_int,     # order_action_ref
    ctypes.c_int      # req_iinfo
]
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
    print(f"[App_entry] Params parsed to cpp is {json_str}, type of user_input is {type(user_input)}")
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
            rtn["StrategyGroup"].append(model_instance)

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



########################################################
class OrderLimitPriceRequest(BaseModel):
    exchange_id: str  # Will be converted to c_char
    volume: int       # Matches c_int
    price: float      # Will be converted to c_double
    stock_id: str     # Will be converted to c_char_p
    req_sinfo: str    # Will be converted to c_char_p
    order_ref: int    # Matches c_int
    req_iinfo: int    # Matches c_int
class CancelOrderRequest(BaseModel):
    exchange_id: str  # Ensure it's a single character string
    req_sinfo: str
    order_ref: int
    order_action_ref: int
    req_iinfo: int

@app.post('/manuel_Send_Order_LimitPrice')
def manuel_Send_Order_LimitPrice(order: OrderLimitPriceRequest):
    exchange_id = ctypes.c_char(order.exchange_id.encode('utf-8')[0])  # Taking first char
    volume = ctypes.c_int(order.volume)
    price = ctypes.c_double(order.price)
    stock_id = ctypes.c_char_p(order.stock_id.encode('utf-8'))
    req_sinfo = ctypes.c_char_p(order.req_sinfo.encode('utf-8'))
    order_ref = ctypes.c_int(order.order_ref)
    req_iinfo = ctypes.c_int(order.req_iinfo)
    mylib.manual_Send_Order_LimitPrice(exchange_id, volume , price , stock_id , req_sinfo , order_ref , req_iinfo)


@app.post('/manuel_Send_Cancle_Order_OrderActionRef')
def manual_Send_Cancle_Order_OrderActionRef(request: CancelOrderRequest):
    exchange_id = ctypes.c_char(request.exchange_id.encode('utf-8')[0])
    req_sinfo = ctypes.c_char_p(request.req_sinfo.encode('utf-8'))
    order_ref = ctypes.c_int(request.order_ref)
    order_action_ref = ctypes.c_int(request.order_action_ref)
    req_iinfo = ctypes.c_int(request.req_iinfo)
    mylib.manual_Send_Cancle_Order_OrderActionRef(exchange_id , req_sinfo , order_ref , order_action_ref , req_iinfo)

################################################
shutdown_requested = False
def termination_handler(signum, frame):
    global shutdown_requested
    print("Shutdown signal received...")
    shutdown_requested = True

def perform_cleanup():
    global yaml_file_path, programCache_data
    print("Performing cleanup...")
    with open(yaml_file_path, "w") as file:
        yaml.dump(programCache_data, file)

################################################


if __name__ == "__main__":
    signal.signal(signal.SIGINT, termination_handler)  # Handle Ctrl+C
    signal.signal(signal.SIGTERM, termination_handler)  # Handle kill command

    uvicorn.run(app,port = 9001)