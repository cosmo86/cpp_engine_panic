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

# Read cache data must be define before initing session states
def read_cached_data():
    res = requests.get(url="http://127.0.0.1:9001/read_cached_data")
    if res.status_code == 200:
        res_dict = res.json()
        print("[read_cached_data]" , res_dict, "len is ",{len(res_dict["StrategyGroup"])})
        if len(res_dict["StrategyGroup"]) >= 1:
            return res_dict["StrategyGroup"]
        else:
            return [UserStrategyModel().model_dump()]
    else:
        st.error('缓存读取失败，请联系管理员', icon='🚨')
# Init Session state

if "strategy_id" not in st.session_state:
    st.session_state.strategy_id = 0
if "running_strategy" not in st.session_state:
    st.session_state.running_strategy = [UserStrategyModel().model_dump()]
if "removed_strategy" not in st.session_state:
    st.session_state.removed_strategy = read_cached_data()
if "strategy_container" not in st.session_state:
    st.session_state.strategy_container = True
if "display_group_add" not in st.session_state:
    st.session_state.display_group_add = False


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
    print("[add_strategy]",user_input,"type is ", type(user_input))
    res = requests.post(url="http://127.0.0.1:9001/add_strategy", data=user_input.model_dump_json())
    if res.status_code == 200:
        check_strategy()
        return 0
    else:
        st.error('策略提交异常', icon='🚨')
        return 1

def remove_strategy(user_input: UserStrategyModel):
    res = requests.post(url="http://127.0.0.1:9001/remove_strategy", data=user_input.model_dump_json())
    if res.status_code == 200:
        st.success('策略删除成功!', icon='✅')
        check_strategy()
    else:
        st.error('策略删除异常', icon='🚨')

def check_strategy(container : st.container = None):
    st.session_state.strategy_container = True
    running_res = requests.get(url="http://127.0.0.1:9001/check_running_strategy")
    removed_res = requests.get(url="http://127.0.0.1:9001/check_removed_strategy")
    if running_res.status_code == 200:
        st.success('运行策略查询成功', icon='✅')
        print("[check_strategy] running_res",running_res.json())
        st.session_state.running_strategy = running_res.json()["StrategyGroup"]
    if removed_res.status_code == 200:
        st.success('删除策略查询成功', icon='✅')
        print("[check_strategy] ",removed_res.json() )
        st.session_state.removed_strategy = removed_res.json()["StrategyGroup"]
    if running_res.status_code != 200:
        print("[check_strategy] running_res",running_res.json()["StrategyGroup"])
        st.error('运行策略查询异常', icon='🚨')
    if removed_res.status_code != 200:
        st.error('删除策略查询异常', icon='🚨')

def delay_strategy(user_input: UserStrategyModel):
    res = requests.post(url="http://127.0.0.1:9001/update_strategy_delay_time", data=user_input.model_dump_json())
    if res.status_code == 200:
        st.success('策略添加延时成功!', icon='✅')
        check_strategy()
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
    limit_volume = submit_container.number_input('封单金额(万)：', min_value=100, step=100)
    cancel_volume = submit_container.number_input('撤单金额(万)：', min_value=100, step=100)
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
    user_remove_strategy.ID = str(delete_strategy_id)
    remove = delete_container.button("删除策略", type="secondary", on_click=remove_strategy,
                                     args=(user_remove_strategy,),
                                     use_container_width=True)
    
    ## Delay strategy
    delay_container = st.container()
    delay_strategy_id = delay_container.number_input('输入策略编号', min_value=0, step=1, key="delay_strategy_id")
    delay_duration = delay_container.number_input('输入触发撤单延时，单位毫秒', min_value=0, step=50,key="delay_duration")
    delay_info = UserStrategyModel()
    delay_info.ID = str(delay_strategy_id)
    delay_info.DelayTime = delay_duration
    delay_container.button('触发撤单延时', on_click= delay_strategy, args=(delay_info,), use_container_width=True)




    if submit:
        st.session_state.strategy_container = True

    

# ---- DF container ----
container = st.container()
container.header('策略管理')
container.markdown("###### 运行策略")

if st.session_state.strategy_container:
    #running_df = pd.DataFrame.from_dict(data=st.session_state.running_strategy)
    #print("running_df",st.session_state.running_strategy)
    running_df = pd.DataFrame(st.session_state.running_strategy)
    print(f"running_df {running_df}")
    running_df['Status'] = running_df['Status'].apply(status_mapping)
    running_df['ExchangeID'] = running_df['ExchangeID'].apply(exchange_mapping)
    container.dataframe(
        running_df,
        column_config={
            'SecurtiyID': '证券代码',
            'ExchangeID': '交易所',
            'BuyTriggerVolume': '封单额',
            'CancelVolume': '撤单额',
            'TargetPosition': '目标仓位（股）',
            'CurrPosition': '已买仓位（股）',
            'Count': '撤单次数',
            'ID': '策略编号',
            "Status": "策略状态",
            "OrderID": "策略委托",
            "DelayTime" : "延迟触发",
            "SecurityName" : "股票名称"
        },
            
        column_order=('ID', 'SecurityID','SecurityName','ExchangeID', 'BuyTriggerVolume', 'CancelVolume', 'TargetPosition', 'CurrPosition', 'DelayTime' ,'Count','Status','OrderID'),
        hide_index=True,
        use_container_width=True
    )
    container.markdown("###### 删除策略")
    ##removed_df = pd.DataFrame.from_dict(data=st.session_state.removed_strategy)
    #print("removed_df" ,st.session_state.removed_strategy)
    removed_df = pd.DataFrame(st.session_state.removed_strategy)
    #print(f"removed_df {removed_df}")
    removed_df['Status'] = removed_df['Status'].apply(status_mapping)
    removed_df['ExchangeID'] = removed_df['ExchangeID'].apply(exchange_mapping)
    container.dataframe(
        removed_df,
        column_config={
            'SecurtiyID': '证券代码',
            'ExchangeID': '交易所',
            'BuyTriggerVolume': '封单额',
            'CancelVolume': '撤单额',
            'TargetPosition': '目标仓位（股）',
            'CurrPosition': '已买仓位（股）',
            'Count': '撤单次数',
            'ID': '策略编号',
            "Status": "策略删除时状态",
            "OrderID": "策略委托",
            "DelayTime" : "延迟触发",
            "SecurityName" : "股票名称"
        },
            
        column_order=('ID', 'SecurityID','SecurityName','ExchangeID', 'BuyTriggerVolume', 'CancelVolume', 'TargetPosition', 'CurrPosition', 'DelayTime' ,'Count','Status','OrderID'),
        hide_index=True,
        use_container_width=True
    )

else:
    st.write(":u7121: 无策略运行")



########### Add Group Strategy ###########

def add_group_strategy(model_instances):
    fail_to_add_models = []
    print(model_instances)
    for i,model in enumerate(model_instances):
        model.Position = 1000
        print("[add_group_strategy]",model, "type is ",type(model))
        res_code = add_strategy(model)
        if res_code ==0:
            pass
        else:
            fail_to_add_models.append(model.SecurityID)
        AddGroup_progressBar.progress( i/len(model_instances) )
    AddGroup_progressBar.empty()
    st.error(f"以下股票添加失败， 代码{fail_to_add_models}")

def row_to_model(row):
    return UserStrategyModel(**row.to_dict())

# ---- Add group container ----
AddGroup_container = st.container()
AddGroup_container.header('添加策略组')

uploaded_group = st.file_uploader("上传策略组", type="csv")
if uploaded_group is not None:
    column_types = {'SecurityID':str,
                'ExchangeID': str,
                'BuyTriggerVolume': int,
                'CancelVolume': int,
                'Position': int,
                'TargetPosition': int,
                'CurrPosition': int,
                'DelayTime': int,
                'MaxTriggerTimes':int,
                'ID':str,
                'Status':int,
                'OrderID':str,
                'SecurityName':str
               }
    # Convert the uploaded file to a pandas DataFrame
    uploaded_group_strategy = pd.read_csv(uploaded_group,dtype = column_types, na_filter=False)
    model_instances = uploaded_group_strategy.apply(row_to_model, axis=1)
    model_instances_for_display = pd.DataFrame([i.model_dump() for i in model_instances.to_list()])
    #print(uploaded_group_strategy)
    st.session_state.display_group_add = True


if st.session_state.display_group_add:
    model_instances_for_display['Status'] = model_instances_for_display['Status'].apply(status_mapping)
    model_instances_for_display['ExchangeID'] = model_instances_for_display['ExchangeID'].apply(exchange_mapping)
    AddGroup_container.dataframe(
        model_instances_for_display,
        column_config={
            'SecurtiyID': '证券代码',
            'ExchangeID': '交易所',
            'BuyTriggerVolume': '封单额',
            'CancelVolume': '撤单额',
            'TargetPosition': '目标仓位（股）',
            'CurrPosition': '已买仓位（股）',
            'Count': '撤单次数',
            'ID': '策略编号',
            "Status": "策略状态",
            "OrderID": "策略委托",
            "DelayTime" : "延迟触发",
            "SecurityName" : "股票名称"
        },
            
        column_order=('ID', 'SecurityID','SecurityName','ExchangeID', 'BuyTriggerVolume', 'CancelVolume', 'TargetPosition', 'CurrPosition', 'DelayTime' ,'Count','Status','OrderID'),
        hide_index=True,
        use_container_width=True
    )
    print("model_instances",model_instances)
    AddGroup_container.button('批量添加', on_click=add_group_strategy, args =(model_instances,))

    progress_text = "批量策略添加进度"
    AddGroup_progressBar = AddGroup_container.progress(0, text=progress_text)
