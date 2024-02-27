from pydantic import BaseModel,validator,ValidationError


#	const char TORA_TSTP_EXD_SSE = '1';
#	const char TORA_TSTP_EXD_SZSE = '2';

class UserStrategyModel(BaseModel):
    SecurityID: str = ""
    ExchangeID: str = ""
    BuyTriggerVolume: int = 0
    CancelVolume: int = 0
    Position: int = 0
    TargetPosition: int = 0
    CurrPosition : int = 0
    DelayTime: int = 500
    MaxTriggerTimes: int = 0 
    ID: str = ""
    Status: int = 0
    OrderID: str = " "
    SecurityName : str = ""

    # Timed logic vars
    LowerTimeLimit: int = 0
    ScoutBuyTriggerCashLim: int = 0
    ScoutMonitorDuration: int = 0
    Cond2Percent: float = 0.0
    Cond2HighTime: int = 0
    Cond2TrackDuration: int = 0
    CancelTriggerVolumeLarge: int = 0
    ScoutProtection: bool = False
    Cond4LowTime: int = 0
    Cond4HighTime: int = 0


    
    @validator('ExchangeID', always=True)
    def check_id_exchange_match(cls, v, values, **kwargs):
        if 'SecurityID' in values and values['SecurityID'].startswith('6') and v == '2':#SZSE
            raise ValueError("SecurityID starting with '6' must match SSE not SZSE")
        elif 'SecurityID' in values and values['SecurityID'].startswith('0') and v == '1':#SSE
            raise ValueError("SecurityID starting with '0' must match SZSE not SSE")
        return v


class UserStrategyGroupModel(BaseModel):
    StrategyGroup: list = []