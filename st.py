import streamlit as st  # web development
import pandas as pd
import requests
from pydantic import  ValidationError

from fastapi_model.dataModels import UserStrategyModel
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
    st.session_state.running_strategy = ''
if "removed_strategy" not in st.session_state:
    st.session_state.removed_strategy = ''
if "strategy_container" not in st.session_state:
    st.session_state.strategy_container = False
if 'chart_data' not in st.session_state:
    st.session_state.chart_data = {'x':[],'y':[]}

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
        check_strategy(st)
    else:
        st.error('策略提交异常', icon='🚨')

def remove_strategy(user_input: UserStrategyModel):
    res = requests.post(url="http://127.0.0.1:9000/remove_strategy", data=user_input.model_dump_json())
    if res.status_code == 200:
        st.success('策略删除成功!', icon='✅')
        check_strategy(st)
        if st.session_state.running_strategy == None:
            st.session_state.strategy_container = False
            st.session_state.strategy_id = 0

    else:
        st.error('策略删除异常', icon='🚨')

def check_strategy(container : st.container = None):
    running_res = requests.get(url="http://127.0.0.1:9000/check_running_strategy")
    removed_res = requests.get(url="http://127.0.0.1:9000/check_removed_strategy")
    if 'ErrorMsg' in running_res.json().keys():
        st.session_state.strategy_container = False
    else:
        st.session_state.running_strategy = running_res.json()
        st.session_state.removed_strategy = removed_res.json()
    if container == None:
        st.success('策略查询成功', icon='✅')
    else:
        container.success('策略查询成功', icon='✅')

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
    limit_volume = submit_container.number_input('封单金额(万)：', min_value=1000, step=1000)
    cancel_volume = submit_container.number_input('撤单金额(万)：', min_value=1000, step=1000)
    position = submit_container.number_input('打板金额(万)：', min_value=0.1, step=100.0)
    count = submit_container.number_input('撤单次数：', min_value=1, step=1)
    try:
        user_strategy = UserStrategyModel(SecurityID = stock_code,ExchangeID = exchange)
    except ValidationError as e:
        st.error('股票跟交易所不匹配', icon='🚨')
        user_strategy = UserStrategyModel()
    user_strategy.LimitVolume = int(limit_volume)
    user_strategy.CancelVolume = int(cancel_volume)
    user_strategy.Position = position
    user_strategy.Count = int(count)
    submit = submit_container.button("提交策略", type="primary", on_click=add_strategy, args=(user_strategy,),
                                     use_container_width=True)
    check = submit_container.button("查询策略", type="secondary", on_click=check_strategy, use_container_width=True)

    ## Delete 
    delete_container = st.container()
    delete_container.subheader('策略删除')
    delete_strategy_id = delete_container.number_input('输入策略编号', min_value=0, step=1)
    user_remove_strategy = UserStrategyModel()
    user_remove_strategy.ID = delete_strategy_id
    remove = delete_container.button("删除策略", type="secondary", on_click=remove_strategy,
                                     args=(user_remove_strategy,),
                                     use_container_width=True)
    
    ## Delay strategy
    delay_container = st.container()
    delay_strategy_id = delay_container.number_input('输入策略编号', min_value=0, step=1)
    delay_strategy_id = delay_container.number_input('输入触发撤单延时，单位毫秒', min_value=0, step=50)




    if submit:
        st.session_state.strategy_container = True

    

