/***********************************************************************
*	@company	上海全创信息科技有限公司
*	@file		demo_trade_stock.cpp
*	@brief		traderapi demo
*	@history	2022-8-26
*	@author		n-sight
*
*	Windows：	1.请确认包.h .cpp 以及 .lib 文件都在相同目录；或者VS项目配置属性中【附加包含目录】以及【附加库目录】和【附加项依赖】正确设置相关路径
*				2.预处理器定义 _CRT_SECURE_NO_WARNINGS ;
*				3.使用VS2013以上版本编译通过
*
*	Linux：		1.库文件和头文件在同一目录下时， g++ demo_trade_stock.cpp -o demo -L. -lfasttraderapi
*				2.当库文件和源文件不在同一目录时，请注意相应路径的不同
				3.运行时若找不到动态库，可export $LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
***********************************************************************/
#pragma once
#include "TORATstpTraderApi.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <map>

#include "concurrentqueue.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "SEObject.hpp"
#include "Event.h"

using LoggerPtr = std::shared_ptr<spdlog::async_logger>;

using namespace TORASTOCKAPI;
/*
1.CTORATstpTraderApi::CreateTstpTraderApi("./flow", false);
2.RegisterSpi(&trade_spi);
3.RegisterFront((char*)TD_TCP_FrontAddress);
4.// 订阅公有流和私有流
	demo_trade_api->SubscribePrivateTopic(TORA_TERT_QUICK);
	demo_trade_api->SubscribePublicTopic(TORA_TERT_QUICK);
5. demo_trade_api->Init();
	
*/

class TradeSpi : public CTORATstpTraderSpi
{
private:
	CTORATstpTraderApi* m_api;
	int m_req_id;
	int m_front_id;
	int m_session_id;

	char m_userid[21];
	char m_password[41];
	char m_address[64];
	char m_mode[21];

	moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr = nullptr;
	LoggerPtr m_logger=nullptr;

	std::unordered_map<std::string, double> limup_table;
	std::unordered_map<TTORATstpExchangeIDType, std::string> shareHolder_table;

public:
	TradeSpi()
	{
		m_api = CTORATstpTraderApi::CreateTstpTraderApi("./flow", false);
		m_req_id = 1;
		std::cout<<"[TradeSpi] TraderApi created"<< std::endl;
	}

	~TradeSpi()
	{}

public:
	void init_queue(moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr)
	{
		std::cout<< "[TradeSpi] pointer before init :"<<m_Event_Q_ptr<<std::endl;
		this->m_Event_Q_ptr = m_Event_Q_ptr;
		std::cout<< "[TradeSpi] pointer to queue inited, address is "<< m_Event_Q_ptr<<std::endl;
	}
	// connect to L2 server
	virtual void connect(const char* const userid, const char* const password, const char* const address, const char* const mode)
	{
		strcpy(m_userid, userid);
		strcpy(m_password, password);
		strcpy(m_address, address);
		strcpy(m_mode, mode);

		m_api->RegisterSpi(this);
		m_api->RegisterFront(m_address);
		std::cout<<"[TradeSpi] RegisterFront"<< std::endl;
		std::cout<<"[TradeSpi] RegisterSpi"<< std::endl;
		m_api->Init();
		std::cout<<"[TradeSpi] Init"<< std::endl;
	}

private:
	virtual void OnFrontConnected()
	{
		std::cout<<"[TradeSpi]  OnFrontConnected"<< std::endl;

		// 获取终端信息
		int ret = m_api->ReqGetConnectionInfo(m_req_id++);
		if (ret != 0)
		{
			printf("[TradeSpi] ReqGetConnectionInfo fail, ret[%d]\n", ret);
		}
	}

	virtual void OnFrontDisconnected(int nReason)
	{
		printf("TradeApi OnFrontDisconnected: [%d]\n", nReason);
	}

	virtual void OnRspGetConnectionInfo(CTORATstpConnectionInfoField* pConnectionInfoField, CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		if (pRspInfo->ErrorID == 0)
		{
			printf("[TradeSpi]  inner_ip_address[%s]\n"
				"[TradeSpi]  inner_port[%d]\n"
				"[TradeSpi]  outer_ip_address[%s]\n"
				"[TradeSpi] outer_port[%d]\n"
				"[TradeSpi] mac_address[%s]\n",
				pConnectionInfoField->InnerIPAddress,
				pConnectionInfoField->InnerPort,
				pConnectionInfoField->OuterIPAddress,
				pConnectionInfoField->OuterPort,
				pConnectionInfoField->MacAddress);


			CTORATstpReqUserLoginField field;
			memset(&field, 0, sizeof(CTORATstpReqUserLoginField));

			// 支持以用户代码、资金账号和股东账号方式登录
			// （1）以用户代码方式登录
			strcpy(field.LogInAccount, m_userid);
			field.LogInAccountType = TORA_TSTP_LACT_UserID;
			strcpy(field.Password, m_password);
			// （2）以资金账号方式登录
			//strcpy(field.DepartmentID, DepartmentID);
			//strcpy(field.LogInAccount, AccountID);
			//field.LogInAccountType = TORA_TSTP_LACT_AccountID;
			// （3）以上海股东账号方式登录
			//strcpy(field.LogInAccount, SSE_ShareHolderID);
			//field.LogInAccountType = TORA_TSTP_LACT_SHAStock;
			// （4）以深圳股东账号方式登录
			//strcpy(field.LogInAccount, SZSE_ShareHolderID);
			//field.LogInAccountType = TORA_TSTP_LACT_SZAStock;

			// 支持以密码和指纹(移动设备)方式认证
			// （1）密码认证
			// 密码认证时AuthMode可不填
			//field.AuthMode = TORA_TSTP_AM_Password;
			// （2）指纹认证
			// 非密码认证时AuthMode必填
			//field.AuthMode = TORA_TSTP_AM_FingerPrint;
			//strcpy(field.DeviceID, "03873902");
			//strcpy(field.CertSerial, "9FAC09383D3920CAEFF039");

			// 终端信息采集
			// UserProductInfo填写终端名称
			strcpy(field.UserProductInfo, "HX5ZJ0C1PV");
			// 按照监管要求填写终端信息
			strcpy(field.TerminalInfo, "PC;IIP=NA;IPORT=NA;LIP=NA;MAC=123ABC456DEF;HD=00b6f88d0890cb7c2b505cb50cb00506;@HX5ZJ0C1PV");
			// 以下内外网IP地址若不填则柜台系统自动采集，若填写则以终端填值为准报送
			//strcpy(field.MacAddress, pConnectionInfoField->MacAddress);
			//strcpy(field.InnerIPAddress, pConnectionInfoField->InnerIPAddress);
			//strcpy(field.OuterIPAddress, pConnectionInfoField->OuterIPAddress);


			int ret = m_api->ReqUserLogin(&field, m_req_id++);
			if (ret != 0)
			{
				printf("[TradeSpi] TradeApi ReqUserLogin fail, ret[%d]\n", ret);
			}
		}
		else
		{
			printf("[TradeSpi] get connection info fail! error_id[%d] error_msg[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	void OnRspUserLogin(CTORATstpRspUserLoginField* pRspUserLoginField, CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		if (pRspInfo->ErrorID == 0)
		{
			printf("[TradeSpi] TradeApi OnRspUserLogin: OK! [%d]\n", nRequestID);

			m_front_id = pRspUserLoginField->FrontID;
			m_session_id = pRspUserLoginField->SessionID;

			// 查询合约
			CTORATstpQrySecurityField field;
			memset(&field, 0, sizeof(field));

			// 以下字段不填表示不设过滤条件，即查询全部合约
			  //field.ExchangeID = TORA_TSTP_EXD_SSE;
			  //strcpy(field.SecurityID, "600000");
			int ret = m_api->ReqQrySecurity(&field, m_req_id++);
			if (ret != 0)
			{
				printf("[TradeSpi] ReqQrySecurity fail, ret[%d]\n", ret);
			}


			// 查询投资者信息
			CTORATstpQryInvestorField field;
			memset(&field, 0, sizeof(field));//不初始化内存将有可能脏数据请求，然后查不到结果

			// 以下字段不填表示不设过滤条件
			//strcpy_s(field.InvestorID, InvestorID);

			int ret = m_api->ReqQryInvestor(&field, m_req_id++);
			if (ret != 0)
			{
				printf("[TradeSpi]  ReqQryInvestor fail, ret[%d]\n", ret);
			}

			// 查询股东账户
			CTORATstpQryShareholderAccountField field_sa;
			memset(&field_sa, 0, sizeof(field_sa));

			// 以下字段不填表示不设过滤条件，即查询所有股东账号
			//field.ExchangeID = TORA_TSTP_EXD_SSE;

			int ret_sa = m_api->ReqQryShareholderAccount(&field_sa, m_req_id++);
			if (ret_sa != 0)
			{
				printf("[TradeSpi]  ReqQryShareholderAccount fail, ret[%d]\n", ret_sa);
			}
		}
	}


	void Send_Order_LimitPrice( const char exchange_id, const int volume, const double price, TTORATstpSecurityIDType stock_id )
	{
		
		// 请求报单
		CTORATstpInputOrderField input_order_field;
		memset(&input_order_field, 0, sizeof(input_order_field));
		input_order_field.ExchangeID = exchange_id;
		strcpy(input_order_field.ShareholderID ,shareHolder_table[exchange_id].c_str());
		strcpy(input_order_field.SecurityID, stock_id);
		input_order_field.Direction = TORA_TSTP_D_Buy;
		input_order_field.VolumeTotalOriginal = volume;
		input_order_field.LimitPrice = price;
		input_order_field.OrderPriceType = TORA_TSTP_OPT_LimitPrice;
		input_order_field.TimeCondition = TORA_TSTP_TC_GFD;
		input_order_field.VolumeCondition = TORA_TSTP_VC_AV;

		int ret_oi = m_api->ReqOrderInsert(&input_order_field, m_req_id++);
		if (ret_oi != 0)
		{
			printf("ReqOrderInsert fail, ret[%d]\n", ret_oi);
		}
	}

	void Send_Cancle_Order( const char exchange_id, TTORATstpOrderSysIDType order_sys_id )
	{
		// 请求撤单
		CTORATstpInputOrderActionField input_order_action_field;
		memset(&input_order_action_field, 0, sizeof(input_order_action_field));
		input_order_action_field.ExchangeID = exchange_id;
		input_order_action_field.ActionFlag = TORA_TSTP_AF_Delete;
		strcpy(input_order_action_field.OrderSysID, order_sys_id);


		int ret = m_api->ReqOrderAction(&input_order_action_field, m_req_id++);
		if (ret != 0)
		{
			printf("ReqOrderAction fail, ret[%d]\n", ret);
		}
	}

#if 0
			// 查询集中交易资金
			CTORATstpReqInquiryJZFundField field;

			memset(&field, 0, sizeof(field));
			strcpy(field.DepartmentID, DepartmentID);
			strcpy(field.AccountID, AccountID);
			field.CurrencyID = TORA_TSTP_CID_CNY;

			int ret = m_api->ReqInquiryJZFund(&field, m_req_id++);
			if (ret != 0)
			{
				printf("ReqInquiryJZFund fail, ret[%d]\n", ret);
			}

#endif

#if 0
			// 资金转移(包括资金调拨和银证转账)
			CTORATstpInputTransferFundField field;

			memset(&field, 0, sizeof(field));
			strcpy(field.DepartmentID, DepartmentID);
			strcpy(field.AccountID, AccountID);
			field.CurrencyID = TORA_TSTP_CID_CNY;

			field.Amount = 100000.0;

			// 转移方向：
			// TORA_TSTP_TRNSD_MoveIn表示资金从集中交易柜台调拨至快速交易柜台
			// TORA_TSTP_TRNSD_MoveOut表示资金从快速交易柜台调拨至集中交易柜台
			// TORA_TSTP_TRNSD_StockToBank表示证券快速交易系统资金转入银行，即出金
			// TORA_TSTP_TRNSD_BankToStock表示银行资金转入证券快速交易系统，即入金
			// 以下说明各场景下字段填值：
			// （1）资金从集中交易柜台调拨至快速交易柜台
			field.TransferDirection = TORA_TSTP_TRNSD_MoveIn;
			// （2）资金从快速交易柜台调拨至集中交易柜台
			//field.TransferDirection = TORA_TSTP_TRNSD_MoveOut;
			// （3）证券快速交易系统资金转入银行，需填写银行代码和资金密码
			//field.TransferDirection = TORA_TSTP_TRNSD_StockToBank;
			//field.BankID = TORA_TSTP_BKID_CCB;
			//strcpy(field.AccountPassword, "123456");
			// （4）银行资金转入证券快速交易系统，需填写银行代码和银行卡密码
			//field.TransferDirection = TORA_TSTP_TRNSD_BankToStock;
			//field.BankID = TORA_TSTP_BKID_CCB;
			//strcpy(field.BankPassword, "123456");

			// 申请流水号ApplySerial字段为选填字段
			// 若不填写则柜台系统会自动生成一个申请流水号
			// 若填写则需保证同一个TCP会话下申请流水号不重复
			//field.ApplySerial = 1;

			int ret = m_api->ReqTransferFund(&field, m_req_id++);
			if (ret != 0)
			{
				printf("ReqTransferFund fail, ret[%d]\n", ret);
			}
#endif

#if 0
			// 登出,目前登出成功连接会立即被柜台系统断开，终端不会收到OnRspUserLogout应答
			// 连接断开后接口内部会触发重新连接，为不使连接成功后又触发重新登录，需终端做好逻辑控制
			// 一般情况下若希望登出，直接调用Release接口即可，释放成功连接将被终端强制关闭，Release接口调用注意事项见下文说明
			CTORATstpUserLogoutField field;
			memset(&field, 0, sizeof(field));

			int ret = m_api->ReqUserLogout(&field, ++m_req_id);
			if (ret != 0)
			{
				printf("ReqUserLogout fail, ret[%d]\n", ret);
			}
#endif
			return;
		}
		else
		{
			printf("TradeApi OnRspUserLogin: Error! [%d] [%d] [%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}



	virtual void OnRspOrderInsert(CTORATstpInputOrderField* pInputOrderField, CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		if (pRspInfo->ErrorID == 0)
		{
			printf("OnRspOrderInsert: OK! [%d] [%d] [%s]\n", nRequestID, pInputOrderField->OrderRef, pInputOrderField->OrderSysID);
		}
		else
		{
			printf("OnRspOrderInsert: Error! [%d] [%d] [%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspOrderAction(CTORATstpInputOrderActionField* pInputOrderActionField, CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		if (pRspInfo->ErrorID == 0)
		{
			printf("OnRspOrderAction: OK! [%d] [%d] [%s] \n", nRequestID, pInputOrderActionField->OrderActionRef, pInputOrderActionField->CancelOrderSysID);
		}
		else
		{
			printf("OnRspOrderAction: Error! [%d] [%d] [%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}


	virtual void OnRtnOrder(CTORATstpOrderField* pOrder)
	{
		printf(
			"OnRtnOrder:::\n"
			"---RequestID[%d] SecurityID[%s] OrderRef[%d] OrderLocalID[%s] OrderSysID[%s]\n"
			"---OrderType[%c] LimitPrice[%.2f]\n"
			"---OrderStatus[%c] StatusMsg[%s] OrderSubmitStatus[%c]\n"
			"---VolumeTotalOriginal[%d] VolumeTraded[%d] VolumeCanceled[%d]\n"
			"---InsertUser[%s] InsertDate[%s] InsertTime[%s] AcceptTime[%s]\n"
			"---CancelUser[%s] CancelTime[%s]"
			"---PbuID[%s]"
			"\n"
			, pOrder->RequestID, pOrder->SecurityID, pOrder->OrderRef, pOrder->OrderLocalID, pOrder->OrderSysID
			, pOrder->OrderType, pOrder->LimitPrice
			, pOrder->OrderStatus, pOrder->StatusMsg, pOrder->OrderSubmitStatus
			, pOrder->VolumeTotalOriginal, pOrder->VolumeTraded, pOrder->VolumeCanceled
			, pOrder->InsertUser, pOrder->InsertDate, pOrder->InsertTime, pOrder->AcceptTime
			, pOrder->CancelUser, pOrder->CancelTime
			, pOrder->PbuID
		);

		//撤单测试
#if 1
	// 请求撤单  交易所已接收
		if (TORA_TSTP_OST_Accepted == pOrder->OrderStatus && TORA_TSTP_OSS_InsertSubmitted == pOrder->OrderSubmitStatus)
		{

			CTORATstpInputOrderActionField field;
			memset(&field, 0, sizeof(CTORATstpInputOrderActionField));

			field.ExchangeID = pOrder->ExchangeID;
			field.ActionFlag = TORA_TSTP_AF_Delete;


			// 撤单支持以下两种方式定位原始报单：
			// （1）报单引用方式
			//field.OrderRef = 1;
			//field.FrontID = m_front_id;
			//field.SessionID = m_session_id;
			// （2）系统报单编号方式
			strcpy(field.OrderSysID, pOrder->OrderSysID);


			// OrderActionRef报单操作引用，用法同报单引用，可根据需要选填

			// 终端自定义字段，终端可根据需要填写如下字段的值，该字段值不会被柜台系统修改，在查询撤单时返回给终端
			//strcpy(field.SInfo, "sinfo");
			//field.IInfo = 678;

			// 委托方式字段根据券商要求填写，无特殊说明置空即可
			//Operway

			int ret = m_api->ReqOrderAction(&field, m_req_id++);
			if (ret != 0)
			{
				printf("ReqOrderAction fail, ret[%d]\n", ret);
			}
		} //end if
#endif


	}

	virtual void OnRtnTrade(CTORATstpTradeField* pTrade)
	{
		printf("OnRtnTrade: TradeID[%s] InvestorID[%s] SecurityID[%s] OrderRef[%d] OrderLocalID[%s] Price[%.2f] Volume[%d]\n",
			pTrade->TradeID, pTrade->InvestorID, pTrade->SecurityID, pTrade->OrderRef, pTrade->OrderLocalID, pTrade->Price, pTrade->Volume);
	}

	virtual void OnRtnMarketStatus(CTORATstpMarketStatusField* pMarketStatus)
	{
		printf("OnRtnMarketStatus: MarketID[%c] MarketStatus[%c]\n", pMarketStatus->MarketID, pMarketStatus->MarketStatus);
	}

	virtual void OnRspQrySecurity(CTORATstpSecurityField* pSecurity, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pSecurity)
		{
			limup_table.emplace(std::string(pSecurity->SecurityID), pSecurity->UpperLimitPrice);
		}
		if (bIsLast)
		{
			std::cout<<"[TraderSpi] 600519 limupPrice is "<<limup_table["600519"]<<std::endl;
		}
	}

	virtual void OnRspQryInvestor(CTORATstpInvestorField* pInvestor, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestor)
		{
			printf("OnRspQryInvestor[%d]: InvestorID[%s] InvestorName[%s] Operways[%s]\n", nRequestID, pInvestor->InvestorID, pInvestor->InvestorName, pInvestor->Operways);
		}

		if (bIsLast)
		{
			printf("查询投资者结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryShareholderAccount(CTORATstpShareholderAccountField* pShareholderAccount, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pShareholderAccount)
		{
			shareHolder_table.emplace(pShareholderAccount->ExchangeID,std::string(pShareholderAccount->ShareholderID));
			printf("OnRspQryShareholderAccount[%d]: InvestorID[%s] ExchangeID[%c] ShareholderID[%s]\n",
				nRequestID,
				pShareholderAccount->InvestorID,
				pShareholderAccount->ExchangeID,
				pShareholderAccount->ShareholderID);
		}

		if (bIsLast)
		{
			printf("查询股东账户结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryTradingAccount(CTORATstpTradingAccountField* pTradingAccount, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingAccount)
		{
			printf("OnRspQryTradingAccount[%d]: DepartmentID[%s] InvestorID[%s] AccountID[%s] CurrencyID[%c] UsefulMoney[%.2f] FetchLimit[%.2f]\n", nRequestID,
				pTradingAccount->DepartmentID, pTradingAccount->InvestorID, pTradingAccount->AccountID, pTradingAccount->CurrencyID,
				pTradingAccount->UsefulMoney, pTradingAccount->FetchLimit);
		}

		if (bIsLast)
		{
			printf("查询资金账户结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryOrder(CTORATstpOrderField* pOrder, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pOrder)
		{
			printf(" OnRspQryOrder[%d]:SecurityID[%s] OrderLocalID[%s] OrderRef[%d] OrderSysID[%s] VolumeTraded[%d] OrderStatus[%c] OrderSubmitStatus[%c] StatusMsg[%s]\n",
				nRequestID,
				pOrder->SecurityID,
				pOrder->OrderLocalID, pOrder->OrderRef, pOrder->OrderSysID, pOrder->VolumeTraded,
				pOrder->OrderStatus, pOrder->OrderSubmitStatus, pOrder->StatusMsg);
		}

		if (bIsLast)
		{
			printf("查询报单结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryPosition(CTORATstpPositionField* pPosition, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pPosition)
		{
			printf("OnRspQryPosition[%d]: InvestorID[%s] SecurityID[%s] HistoryPos[%d] TodayBSPos[%d] TodayPRPos[%d] AvailablePosition[%d] CurrentPosition[%d]\n", nRequestID, pPosition->InvestorID, pPosition->SecurityID, pPosition->HistoryPos, pPosition->TodayBSPos, pPosition->TodayPRPos, pPosition->AvailablePosition, pPosition->CurrentPosition);
		}

		if (bIsLast)
		{
			printf("查询持仓结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}


};



int main(int argc, char* argv[])
{
	// 打印接口版本号
	printf("TradeApiVersion:[%s]\n", CTORATstpTraderApi::GetApiVersion());

	// 创建接口对象
	// pszFlowPath为私有流和公有流文件存储路径，若订阅私有流和公有流且创建多个接口实例，每个接口实例应配置不同的路径
	// bEncrypt为网络数据是否加密传输，考虑数据安全性，建议以互联网方式接入的终端设置为加密传输
	CTORATstpTraderApi* demo_trade_api = CTORATstpTraderApi::CreateTstpTraderApi("./flow", false);

	// 创建回调对象
	DemoTradeSpi trade_spi(demo_trade_api);

	// 注册回调接口
	demo_trade_api->RegisterSpi(&trade_spi);


#if 1	//模拟环境，TCP 直连Front方式
	// 注册单个交易前置服务地址
	const char* TD_TCP_FrontAddress = "tcp://210.14.72.21:4400";//仿真交易环境
	//const char* TD_TCP_FrontAddress="tcp://210.14.72.15:4400";//24小时环境A套
	//const char* TD_TCP_FrontAddress="tcp://210.14.72.16:9500";////24小时环境B套
	demo_trade_api->RegisterFront((char*)TD_TCP_FrontAddress);
	// 注册多个交易前置服务地址，用逗号隔开
	//demo_trade_api->RegisterFront("tcp://10.0.1.101:6500,tcp://10.0.1.101:26500");
	printf("TD_TCP_FensAddress[sim or 24H]::%s\n", TD_TCP_FrontAddress);

#else	//模拟环境，FENS名字服务器方式
	const char* TD_TCP_FensAddress = "tcp://210.14.72.21:42370";//模拟环境通用fens地址
	/********************************************************************************
	 * 注册 fens 地址前还需注册 fens 用户信息，包括环境编号、节点编号、Fens 用户代码等信息
	 * 使用名字服务器的好处是当券商系统部署方式发生调整时外围终端无需做任何前置地址修改
	 * *****************************************************************************/
	CTORATstpFensUserInfoField fens_user_info_field;
	memset(&fens_user_info_field, 0, sizeof(fens_user_info_field));
	strcpy(fens_user_info_field.FensEnvID, "stock");//必填项，暂时固定为“stock”表示普通现货柜台
	strcpy(fens_user_info_field.FensNodeID, "sim");//必填项，生产环境需按实际填写,仿真环境为sim
	//strcpy(fens_user_info_field.FensNodeID, "24a");//必填项，生产环境需按实际填写,24小时A套环境为24a
	//strcpy(fens_user_info_field.FensNodeID, "24b");//必填项，生产环境需按实际填写,24小时B套环境为24b
	demo_trade_api->RegisterFensUserInfo(&fens_user_info_field);
	demo_trade_api->RegisterNameServer((char*)TD_TCP_FensAddress);
	printf("TD_TCP_FensAddress[%s]::%s\n", fens_user_info_field.FensNodeID, TD_TCP_FensAddress);

#endif




	// 订阅公有流和私有流
	demo_trade_api->SubscribePrivateTopic(TORA_TERT_QUICK);
	demo_trade_api->SubscribePublicTopic(TORA_TERT_QUICK);
	/*	************************************
	*	TORA_TERT_RESTART，从日初开始
	*	TORA_TERT_RESUME,  从断开时候开始
	*	TORA_TERT_QUICK ，从最新时刻开始
	****************************************/

	// 启动
	demo_trade_api->Init();

	// 等待结束
	getchar();
	//demo_trade_api->Join();

	// 释放，注意不允许在回调函数内调用Release接口，否则会导致线程死锁
	demo_trade_api->Release();

	return 0;
}