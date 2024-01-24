from fastapi_model.dataModels import UserStrategyModel
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
print(f"type is {type(data)}, data:{ data['BuyTriggerVolume']}")

model_instance = UserStrategyModel(**data)
print(model_instance)
print(model_instance.BuyTriggerVolume)