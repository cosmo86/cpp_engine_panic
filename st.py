import streamlit as st  # web development
import pandas as pd
import requests
from pydantic import  ValidationError

from fastapi_model.dataModels import UserStrategyModel,UserStrategyGroupModel
from fastapi_model.st_mappings import ORDER_STATUS_TORA2ST, EXCHANGE_MAPPING_TORA2ST

#Helper function
def status_mapping(row):
    return ORDER_STATUS_TORA2ST[row]
def exchange_mapping(row):
    return EXCHANGE_MAPPING_TORA2ST[row]


# Init Session state

if "strategy_id" not in st.session_state:
    st.session_state.strategy_id = 0
if "running_strategy" not in st.session_state:
    st.session_state.running_strategy = UserStrategyGroupModel().model_dump()
if "removed_strategy" not in st.session_state:
    st.session_state.removed_strategy = UserStrategyGroupModel().model_dump()
if "strategy_container" not in st.session_state:
    st.session_state.strategy_container = False


# Page Title
st.set_page_config(
    page_title='领桥量化',
    page_icon='✅',
    layout='wide'
)

# Dashboard title
st.title("程序化盘中跟板策略客户端v0.0.1")

st.sidebar.header("策略编辑")


def add_strategy(user_input: UserStrategyModel):
    res = requests.post(url="http://127.0.0.1:9000/add_strategy", data=user_input.model_dump_json())
    if res.status_code == 200:
        check_strategy()
    else:
        st.error('策略提交异常', icon='🚨')

def remove_strategy(user_input: UserStrategyModel):
    res = requests.post(url="http://127.0.0.1:9000/remove_strategy", data=user_input.model_dump_json())
    if res.status_code == 200:
        st.success('策略删除成功!', icon='✅')
        check_strategy()
        if st.session_state.running_strategy == None:
            st.session_state.strategy_container = False
            st.session_state.strategy_id = 0

    else:
        st.error('策略删除异常', icon='🚨')

def check_strategy(container : st.container = None):
    st.session_state.strategy_container = True
    running_res = requests.get(url="http://127.0.0.1:9000/check_running_strategy")
    removed_res = requests.get(url="http://127.0.0.1:9000/check_removed_strategy")
    if running_res.status_code == 200:
        st.success('运行策略查询成功', icon='✅')
        st.session_state.running_strategy = running_res.json()
    elif removed_res.status_code == 200:
        st.success('删除策略查询成功', icon='✅')
        st.session_state.removed_strategy = removed_res.json()
    elif running_res.status_code != 200:
        st.error('运行策略查询异常', icon='🚨')
    else:
        st.error('删除策略查询异常', icon='🚨')

def delay_strategy(user_input: UserStrategyModel):
    res = requests.post(url="http://127.0.0.1:9000/update_strategy_delay_time", data=user_input.model_dump_json())
    if res.status_code == 200:
        st.success('策略添加延时成功!', icon='✅')
    else:
        st.error('策略添加延时异常', icon='🚨')

## Side Bar ##

with st.sidebar:
    ## Submit 
    submit_container = st.container()
    submit_container.subheader('策略提交')
    stock_code = submit_container.text_input('输入股票代码(6位数字)：', placeholder='600000')
    exchange = submit_container.selectbox('选择交易所：', ('SSE', 'SZSE'))
    if exchange == 'SSE':
        exchange='1'
    if exchange == 'SZSE':
        exchange = '2'
    limit_volume = submit_container.number_input('封单金额(万)：', min_value=1000, step=1000)
    cancel_volume = submit_container.number_input('撤单金额(万)：', min_value=1000, step=1000)
    position = submit_container.number_input('打板金额(万)：', min_value=0.1, step=100.0)
    count = submit_container.number_input('撤单次数：', min_value=1, step=1)
    try:
        user_strategy = UserStrategyModel(SecurityID = stock_code,ExchangeID = exchange)
    except ValidationError as e:
        st.error('股票跟交易所不匹配', icon='🚨')
        user_strategy = UserStrategyModel()
    user_strategy.BuyTriggerVolume = int(limit_volume *10_000)
    user_strategy.CancelVolume = int(cancel_volume * 10_000)
    user_strategy.Position = int(position * 10_000)
    user_strategy.MaxTriggerTimes = int(count)
    submit = submit_container.button("提交策略", type="primary", on_click=add_strategy, args=(user_strategy,),
                                     use_container_width=True)
    check = submit_container.button("查询策略", type="secondary", on_click=check_strategy, use_container_width=True)

    ## Delete 
    delete_container = st.container()
    delete_container.subheader('策略删除')
    delete_strategy_id = delete_container.number_input('输入策略编号', min_value=0, step=1, key="Delete")
    user_remove_strategy = UserStrategyModel()
    user_remove_strategy.ID = delete_strategy_id
    remove = delete_container.button("删除策略", type="secondary", on_click=remove_strategy,
                                     args=(user_remove_strategy,),
                                     use_container_width=True)
    
    ## Delay strategy
    delay_container = st.container()
    delay_strategy_id = delay_container.number_input('输入策略编号', min_value=0, step=1, key="delay_strategy_id")
    delay_duration = delay_container.number_input('输入触发撤单延时，单位毫秒', min_value=0, step=50,key="delay_duration")




    if submit:
        st.session_state.strategy_container = True

    

# ---- DF container ----
container = st.container()
container.header('策略管理')
container.markdown("###### 运行策略")
if st.session_state.strategy_container:
    running_df = pd.DataFrame.from_dict(data=st.session_state.running_strategy['StrategyGroup'])
    running_df['Status'] = running_df['Status'].apply(status_mapping)
    running_df['ExchangeID'] = running_df['ExchangeID'].apply(exchange_mapping)
    container.dataframe(
        running_df,
        column_config={
            'SecurtiyID': '证券代码',
            'ExchangeID': '交易所',
            'LimitVolume': '封单额',
            'CancelVolume': '撤单额',
            'TargetPosition': '目标仓位（股）',
            'CurrPosition': '已买仓位（股）',
            'Count': '撤单次数',
            'ID': '策略编号',
            "Status": "策略状态",
            "OrderID": "策略委托",
            "DelayTime" : "延迟触发",
            "Name" : "股票名称"
        },
            
        column_order=('ID', 'SecurityID','Name','ExchangeID', 'LimitVolume', 'CancelVolume', 'TargetPosition', 'CurrPosition', 'DelayTime' ,'Count','Status','OrderID'),
        hide_index=True,
        use_container_width=True
    )
    container.markdown("###### 删除策略")
    removed_df = pd.DataFrame.from_dict(data=st.session_state.removed_strategy['StrategyGroup'])
    removed_df['Status'] = removed_df['Status'].apply(status_mapping)
    removed_df['ExchangeID'] = removed_df['ExchangeID'].apply(exchange_mapping)
    container.dataframe(
        removed_df,
        column_config={
            'SecurtiyID': '证券代码',
            'ExchangeID': '交易所',
            'LimitVolume': '封单额',
            'CancelVolume': '撤单额',
            'TargetPosition': '目标仓位（股）',
            'CurrPosition': '已买仓位（股）',
            'Count': '撤单次数',
            'ID': '策略编号',
            "Status": "策略删除时状态",
            "OrderID": "策略委托",
            "DelayTime" : "延迟触发",
            "Name" : "股票名称"
        },
            
        column_order=('ID', 'SecurityID','Name','ExchangeID', 'LimitVolume', 'CancelVolume', 'TargetPosition', 'CurrPosition', 'DelayTime' ,'Count','Status','OrderID'),
        hide_index=True,
        use_container_width=True
    )

else:
    st.write(":u7121: 无策略运行")