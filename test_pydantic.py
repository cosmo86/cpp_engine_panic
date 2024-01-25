from fastapi_model.dataModels import UserStrategyModel,UserStrategyGroupModel
import json

data = {
  "SecurityID": "",
  "ExchangeID": "",
  "BuyTriggerVolume": 0,
  "CancelVolume": 0,
  "Position": 0,
  "TargetPosition": 0,
  "CurrPosition": 0,
  "DelayTime": 0,
  "MaxTriggerTimes": 0,
  "ID": "",
  "Status": 0,
  "OrderID": " ",
  "SecurityName": ""
}

model_instance = UserStrategyGroupModel().model_dump()
print(model_instance['StrategyGroup'])