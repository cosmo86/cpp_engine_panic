/***********************************************************************
*	@history	2024-01-01
*	@author		CosmoW
*   @contact    Earth
***********************************************************************/

#pragma once
#include "TORATstpTraderApi.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <map>

#include "iconv.h"
#include "concurrentqueue.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "SEObject.hpp"
#include "Event.h"
#include "OrderModels.hpp"
#include "helper_functions.hpp"

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

// Helper function
inline std::string convertEncoding(const char* originalStr, const char* fromEncoding, const char* toEncoding) {
    iconv_t conv = iconv_open(toEncoding, fromEncoding);
    if (conv == (iconv_t)-1) {
        // Handle error
        return "";
    }

    size_t originalLen = strlen(originalStr);
    size_t outputBufferSize = originalLen * 3 + 1;  // Estimate size needed for UTF-8
    std::vector<char> outputBuffer(outputBufferSize);
    char* outputPtr = outputBuffer.data();

    // Create a temporary pointer for iconv
    char* tempInputPtr = const_cast<char*>(originalStr);

    size_t result = iconv(conv, &tempInputPtr, &originalLen, &outputPtr, &outputBufferSize);
    if (result == (size_t)-1) {
        // Handle conversion error
        iconv_close(conv);
        return "";
    }

    iconv_close(conv);
    return std::string(outputBuffer.data(), outputPtr - outputBuffer.data());
}




class TradeSpi : public CTORATstpTraderSpi
{
private:
	CTORATstpTraderApi* m_api = nullptr;
	int m_req_id;
	int m_front_id;
	int m_session_id;

	char m_userid[21];
	char m_password[41];
	char m_address[64];
	char m_mode[21];

	char m_InvestorID[16];
	
	moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr = nullptr;
	LoggerPtr m_logger=nullptr;

	std::unordered_map<std::string, double> limup_table; //<securityid , limup_price>
	std::unordered_map<std::string, std::string> secID_name_table; //<securityid, SecurityName>
	std::unordered_map<TTORATstpExchangeIDType, std::string> shareHolder_table; // <exchangeid , shareholderid>
	std::unordered_map<std::string, char[33]> OrderSysid_Sinfo_map; //<OrderSysid , Sinfo>

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

	void Release()
	{
		if(m_api){
			m_api->Release();
		}
		else{
			std::cout<<"trade_api not created."<<std::endl;
		}
	}

	void init_trader(moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr,LoggerPtr logger_ptr)
	{
		std::cout<< "[TradeSpi] pointer before init :"<<m_Event_Q_ptr<<std::endl;
		this->m_Event_Q_ptr = m_Event_Q_ptr;
		this->m_logger = logger_ptr;
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

	virtual void OnRspGetConnectionInfo(CTORATstpConnectionInfoField* pConnectionInfoField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID)
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


			TORASTOCKAPI::CTORATstpReqUserLoginField field;
			memset(&field, 0, sizeof(TORASTOCKAPI::CTORATstpReqUserLoginField));

			// 支持以用户代码、资金账号和股东账号方式登录
			// （1）以用户代码方式登录
			strcpy(field.LogInAccount, m_userid);
			field.LogInAccountType = TORASTOCKAPI::TORA_TSTP_LACT_UserID;
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

	void OnRspUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField* pRspUserLoginField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		if (pRspInfo->ErrorID == 0)
		{
			printf("[TradeSpi] TradeApi OnRspUserLogin: OK! [%d]\n", nRequestID);

			m_front_id = pRspUserLoginField->FrontID;
			m_session_id = pRspUserLoginField->SessionID;

			// 查询合约
			CTORATstpQrySecurityField field_Security;
			memset(&field_Security, 0, sizeof(field_Security));

			// 以下字段不填表示不设过滤条件，即查询全部合约
			  //field.ExchangeID = TORA_TSTP_EXD_SSE;
			  //strcpy(field.SecurityID, "600000");
			int ret = m_api->ReqQrySecurity(&field_Security, m_req_id++);
			if (ret != 0)
			{
				printf("[TradeSpi] ReqQrySecurity fail, ret[%d]\n", ret);
			}


			// 查询投资者信息
			CTORATstpQryInvestorField field_Incestor;
			memset(&field_Incestor, 0, sizeof(field_Incestor));//不初始化内存将有可能脏数据请求，然后查不到结果

			// 以下字段不填表示不设过滤条件
			//strcpy_s(field.InvestorID, InvestorID);

			int ret_2 = m_api->ReqQryInvestor(&field_Incestor, m_req_id++);
			if (ret_2 != 0)
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




	virtual void OnRspOrderInsert(CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		if (pRspInfo->ErrorID == 0)
		{
			//printf("OnRspOrderInsert: OK! [%d] [%d] [%s]\n", nRequestID, pInputOrderField->OrderRef, pInputOrderField->OrderSysID);
		}
		else
		{
			//printf("OnRspOrderInsert: Error! [%d] [%d] [%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnErrRtnOrderInsert(CTORATstpInputOrderField *pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField *pRspInfoField, int nRequestID)
	{
		if(pInputOrderField->SInfo[0] == '\0'){
			std::cout<<"[Trader:OnErrRtnOrderInsert] SInfo is empty, please check order source "<< std::endl;
			std::cout<<pInputOrderField->SecurityID<<" "<< pInputOrderField->Direction<<" OrdersysID: "<<pInputOrderField->OrderSysID<<std::endl;
			return;
		}

		SEEvent temp_event;
		std::shared_ptr<SE_InputOrderField> temp_InputOrderField = SEObject::Create<SE_InputOrderField>();
		memcpy(temp_InputOrderField.get(),pInputOrderField,sizeof(SE_InputOrderField));
		temp_event.e_type = Eventtype::ORDER_ERROR;
		temp_event.event = temp_InputOrderField;
		strcpy(temp_event.S_id, pInputOrderField->SInfo);
		m_Event_Q_ptr->enqueue(std::move(temp_event));

	}

	virtual void OnRspOrderAction(CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID)
	{
		printf("[TRADER OnRspOrderAction]  [%d] [%d] [%s] \n", nRequestID, pInputOrderActionField->OrderActionRef, pInputOrderActionField->CancelOrderSysID);
		if(pInputOrderActionField->SInfo[0] == '\0'){
			//std::cout<<"[Trader:OnRspOrderAction] SInfo is empty, please check order source "<< std::endl;
			//printf("[%d] [%d] [%s] \n", nRequestID, pInputOrderActionField->OrderActionRef, pInputOrderActionField->CancelOrderSysID);
			return;
		}
		SEEvent temp_event;
		std::shared_ptr<SE_InputOrderActionField> InputOrderActionField = SEObject::Create<SE_InputOrderActionField>();
		memcpy(InputOrderActionField.get(),pInputOrderActionField,sizeof(SE_InputOrderActionField));

		if (pRspInfo->ErrorID == 0)
		{
			//printf("OnRspOrderAction: OK! [%d] [%d] [%s] \n", nRequestID, pInputOrderActionField->OrderActionRef, pInputOrderActionField->CancelOrderSysID);
			temp_event.e_type = Eventtype::CANCEL_SUCCESS;
			temp_event.event = InputOrderActionField;
			strcpy(temp_event.S_id, pInputOrderActionField->SInfo);
			m_Event_Q_ptr->enqueue(std::move(temp_event));
		}
		else
		{
			//printf("OnRspOrderAction: Error! [%d] [%d] [%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			temp_event.e_type = Eventtype::CANCEL_ERROR;
			temp_event.event = InputOrderActionField;
			strcpy(temp_event.S_id, pInputOrderActionField->SInfo);
			m_Event_Q_ptr->enqueue(std::move(temp_event));
		}
	}


	virtual void OnRtnOrder(CTORATstpOrderField* pOrder)
	{
		m_logger->warn("T,{}, [OnRtnOrder] ,{},{},{},{}",
		pOrder->SInfo,
		pOrder->SecurityID,
		pOrder->OrderSysID,
		pOrder->OrderStatus, 
		pOrder->OrderSubmitStatus);
		printf(
			"[TRADER OnRtnOrder]:::\n"
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

		if(pOrder->SInfo[0] == '\0'){
			std::cout<<"[Trader:OnRtnOrder] SInfo is empty, please check order source "<< std::endl;
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
			return;
		}

		// order succedd status=2 and not withdraw submitstatud = 1
		if (pOrder->OrderStatus == '2' &&  pOrder->OrderSubmitStatus == '1')
		{
			SEEvent temp_event;
			std::shared_ptr<SE_OrderField> orderField = SEObject::Create<SE_OrderField>();
			memcpy(orderField.get(),pOrder,sizeof(SE_OrderField));

			temp_event.e_type = Eventtype::ORDER_SUCCESS;
			temp_event.event = orderField;
			strcpy(temp_event.S_id, pOrder->SInfo);

			//std::shared_ptr<SE_Lev2MarketDataField> temp = std::static_pointer_cast<SE_Lev2MarketDataField>(temp_event.event);
			//std::cout<<temp->SecurityID<<" "<<temp->ClosePrice<<std::endl;
			m_Event_Q_ptr->enqueue(std::move(temp_event));

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
		}
	}

	virtual void OnRtnTrade(CTORATstpTradeField* pTrade)
	{
		printf("[TRADER OnRtnTrade]: TradeID[%s] InvestorID[%s] SecurityID[%s] OrderRef[%d] OrderLocalID[%s] Price[%.2f] Volume[%d]\n",
			pTrade->TradeID, pTrade->InvestorID, pTrade->SecurityID, pTrade->OrderRef, pTrade->OrderLocalID, pTrade->Price, pTrade->Volume);


		auto temp_map_ptr = OrderSysid_Sinfo_map.find(std::string(pTrade->OrderSysID));
		// If ordersysid not in the map, then the order to this trade was not sent by this program, probably done other platform manuelly
		if(temp_map_ptr== OrderSysid_Sinfo_map.end() ){
			std::cout<<"[Trader:OnRtnTrade] SInfo is empty, please check order source "<< std::endl;
			printf("OnRtnTrade: TradeID[%s] InvestorID[%s] SecurityID[%s] OrderRef[%d] OrderLocalID[%s] Price[%.2f] Volume[%d]\n",
			pTrade->TradeID, pTrade->InvestorID, pTrade->SecurityID, pTrade->OrderRef, pTrade->OrderLocalID, pTrade->Price, pTrade->Volume);
			return;
		}

		SEEvent temp_event;
		std::shared_ptr<SE_TradeField> TradeField = SEObject::Create<SE_TradeField>();
		memcpy(TradeField.get(),pTrade,sizeof(SE_TradeField));

		temp_event.e_type = Eventtype::TRADE;
		temp_event.event = TradeField;
		strcpy(temp_event.S_id, temp_map_ptr->second);

		//std::shared_ptr<SE_Lev2MarketDataField> temp = std::static_pointer_cast<SE_Lev2MarketDataField>(temp_event.event);
		//std::cout<<temp->SecurityID<<" "<<temp->ClosePrice<<std::endl;
		m_Event_Q_ptr->enqueue(std::move(temp_event));

		printf("OnRtnTrade: TradeID[%s] InvestorID[%s] SecurityID[%s] OrderRef[%d] OrderLocalID[%s] Price[%.2f] Volume[%d]\n",
			pTrade->TradeID, pTrade->InvestorID, pTrade->SecurityID, pTrade->OrderRef, pTrade->OrderLocalID, pTrade->Price, pTrade->Volume);
	}

	virtual void OnRtnMarketStatus(CTORATstpMarketStatusField* pMarketStatus)
	{
		printf("OnRtnMarketStatus: MarketID[%c] MarketStatus[%c]\n", pMarketStatus->MarketID, pMarketStatus->MarketStatus);
	}

	virtual void OnRspQrySecurity(CTORATstpSecurityField* pSecurity, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pSecurity)
		{
			auto start = std::chrono::high_resolution_clock::now();
			limup_table.emplace(std::string(pSecurity->SecurityID), pSecurity->UpperLimitPrice);
			secID_name_table.emplace(std::string(pSecurity->SecurityID), convertEncoding(pSecurity->SecurityName, "GBK", "UTF-8"));
			auto stop = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
			//std::cout << "[Trader OnRspQrySecurity]: "<< convertEncoding(pSecurity->SecurityName, "GBK", "UTF-8")<<" "<< pSecurity->SecurityName << std::endl;
			//printf("[测试中文 Trader OnRspQrySecurity]: %s\n", pSecurity->SecurityName);
		}
		if (bIsLast)
		{
			std::cout<<"[TraderSpi] size of limup map is  "<<limup_table.size()<<std::endl;
			std::cout<<"[TraderSpi] 600519 name is "<<secID_name_table["600519"]<<std::endl;
		}
	}

	virtual void OnRspQryInvestor(CTORATstpInvestorField* pInvestor, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestor)
		{
			strcpy(m_InvestorID , pInvestor->InvestorID);
			printf("[TraderSpi] OnRspQryInvestor[%d]: InvestorID[%s] InvestorName[%s] Operways[%s]\n", nRequestID, pInvestor->InvestorID, pInvestor->InvestorName, pInvestor->Operways);
		}

		if (bIsLast)
		{
			printf("[TraderSpi] 查询投资者结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryShareholderAccount(CTORATstpShareholderAccountField* pShareholderAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pShareholderAccount)
		{
			shareHolder_table.emplace(pShareholderAccount->ExchangeID,std::string(pShareholderAccount->ShareholderID));
			printf("[TraderSpi] OnRspQryShareholderAccount[%d]: InvestorID[%s] ExchangeID[%c] ShareholderID[%s]\n",
				nRequestID,
				pShareholderAccount->InvestorID,
				pShareholderAccount->ExchangeID,
				pShareholderAccount->ShareholderID);
		}

		if (bIsLast)
		{
			printf("[TraderSpi] 查询股东账户结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryTradingAccount(CTORATstpTradingAccountField* pTradingAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingAccount)
		{
			printf("[TraderSpi] OnRspQryTradingAccount[%d]: DepartmentID[%s] InvestorID[%s] AccountID[%s] CurrencyID[%c] UsefulMoney[%.2f] FetchLimit[%.2f]\n", nRequestID,
				pTradingAccount->DepartmentID, pTradingAccount->InvestorID, pTradingAccount->AccountID, pTradingAccount->CurrencyID,
				pTradingAccount->UsefulMoney, pTradingAccount->FetchLimit);
		}

		if (bIsLast)
		{
			printf("[TraderSpi] 查询资金账户结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryOrder(CTORATstpOrderField* pOrder, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pOrder)
		{
			printf(" [TraderSpi] OnRspQryOrder[%d]:SecurityID[%s] OrderLocalID[%s] OrderRef[%d] OrderSysID[%s] VolumeTraded[%d] OrderStatus[%c] OrderSubmitStatus[%c] StatusMsg[%s]\n",
				nRequestID,
				pOrder->SecurityID,
				pOrder->OrderLocalID, pOrder->OrderRef, pOrder->OrderSysID, pOrder->VolumeTraded,
				pOrder->OrderStatus, pOrder->OrderSubmitStatus, pOrder->StatusMsg);
		}

		if (bIsLast)
		{
			printf("[TraderSpi] 查询报单结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}

	virtual void OnRspQryPosition(CTORATstpPositionField* pPosition, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pPosition)
		{
			printf("[TraderSpi] OnRspQryPosition[%d]: InvestorID[%s] SecurityID[%s] HistoryPos[%d] TodayBSPos[%d] TodayPRPos[%d] AvailablePosition[%d] CurrentPosition[%d]\n", nRequestID, pPosition->InvestorID, pPosition->SecurityID, pPosition->HistoryPos, pPosition->TodayBSPos, pPosition->TodayPRPos, pPosition->AvailablePosition, pPosition->CurrentPosition);
		}

		if (bIsLast)
		{
			printf("[TraderSpi] 查询持仓结束[%d] ErrorID[%d] ErrorMsg[%s]\n", nRequestID, pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
	}
	
public:
	void Send_Order_LimitPrice( const char exchange_id, const int volume, const double price, TTORATstpSecurityIDType stock_id , const char* req_sinfo)
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
		strcpy(input_order_field.SInfo, req_sinfo );
		int ret_oi = m_api->ReqOrderInsert(&input_order_field, m_req_id++);
		if (ret_oi != 0)
		{
			printf("ReqOrderInsert fail, ret[%d]\n", ret_oi);
		}
	}

	void Send_Cancle_Order( const char exchange_id, TTORATstpOrderSysIDType order_sys_id ,const char* req_sinfo )
	{
		// 请求撤单
		CTORATstpInputOrderActionField input_order_action_field;
		memset(&input_order_action_field, 0, sizeof(input_order_action_field));
		input_order_action_field.ExchangeID = exchange_id;
		input_order_action_field.ActionFlag = TORA_TSTP_AF_Delete;
		strcpy(input_order_action_field.OrderSysID, order_sys_id);
		strcpy(input_order_action_field.SInfo , req_sinfo);

		int ret = m_api->ReqOrderAction(&input_order_action_field, m_req_id++);
		if (ret != 0)
		{
			printf("ReqOrderAction fail, ret[%d]\n", ret);
		}
	}

	double get_limup_price(std::string securityID)
	{
		auto limup_table_iter = limup_table.find(securityID);
		if (limup_table_iter != limup_table.end()) {
			return limup_table[securityID];
		} 
		else {return -1.0;}
	}

	std::string get_security_name(std::string securityID)
	{
		auto securityName_table_iter = secID_name_table.find(securityID);
		if (securityName_table_iter != secID_name_table.end()) {
			return secID_name_table[securityID];
		} 
		else {return "NOTFOUND";}
	}

};