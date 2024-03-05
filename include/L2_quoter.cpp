/***********************************************************************
*	@history	2024-01-01
*	@author		CosmoW
*   @contact    Earth
***********************************************************************/


//c++
// CTORATstpLev2MdApi* lev2md_api;

// lev2md_api = CTORATstpLev2MdApi::CreateTstpLev2MdApi();

// lev2md_api->RegisterFront(LEV2MD_TCP_FrontAddress);
// Lev2MdSpi demo_spi(lev2md_api);
// lev2md_api->RegisterSpi(&demo_spi);
// lev2md_api->Init(); //bind cpu cores
// lev2md_api->Release();
#pragma once
#include <stdio.h>
#include <string.h>
#include <map>
#include "TORATstpLev2MdApi.h"
#include "concurrentqueue.h"
#include <jemalloc/jemalloc.h>
#include "Strategy.hpp"
#include "Lv2dataModel.hpp"
#include "Event.h"
#include "SEObject.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "helper_functions.hpp"

using LoggerPtr = std::shared_ptr<spdlog::async_logger>;



#define _CRT_SECURE_NO_WARNINGS
#ifdef  WINDOWS
#pragma comment(lib,"lev2mdapi.lib")
#endif 


using namespace TORALEV2API;


class Lev2MdSpi : public CTORATstpLev2MdSpi
{
public:
	Lev2MdSpi()
	{
	}

	~Lev2MdSpi(void)
	{
	}

public:
	void init_quoter(moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr, LoggerPtr logger_ptr)
	{
		std::cout<< "[Lev2MdSpi] pointer before init :"<<m_Event_Q_ptr<<std::endl;
		this->m_Event_Q_ptr = m_Event_Q_ptr;
		this->m_logger = logger_ptr;
		m_logger->warn( "Q, [Lev2MdSpi] pointer to queue inited address is {}", static_cast<void*>(m_Event_Q_ptr));
	}
	// connect to L2 server
	virtual void connect(const char* const userid, const char* const password, const char* const address, const char* const mode)
	{
		//strncpy(m_userid, userid, sizeof(userid));
		//strncpy(m_password, userid, sizeof(password));
		//strncpy(m_address, userid, sizeof(address));
		//strncpy(m_mode, userid, sizeof(mode));

		strcpy(m_userid, userid);
		strcpy(m_password, password);
		strcpy(m_address, address);
		strcpy(m_mode, mode);
		std::cout<< userid<< " "<<address<<std::endl;
		std::cout<<"[Lev2MdSpi] user ,passwork inited"<< std::endl;
		if (strcmp(mode, "test") == 0)
		{
			std::cout<<"[Lev2MdSpi]  test TCP"<< std::endl;
			m_api = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_TCP,true);
			m_api->RegisterFront(m_address);
		}
		else
		{
			std::cout<<"[Lev2MdSpi]  server udp "<< std::endl;
			m_api = CTORATstpLev2MdApi::CreateTstpLev2MdApi( TORA_TSTP_MST_MCAST,true);
			m_api->RegisterMulticast(m_address,"172.16.2121.1","");
		}
		std::cout<<"[Lev2MdSpi] RegisterFront"<< std::endl;
		m_api->RegisterSpi(this);
		std::cout<<"[Lev2MdSpi] RegisterSpi"<< std::endl;
		m_api->Init();
		std::cout<<"[Lev2MdSpi] Init"<< std::endl;
	}

	//���������ӳɹ��ر�
	virtual void OnFrontConnected()
	{
		std::cout<<"OnFrontConnected!"<<std::endl;
		TORALEV2API::CTORATstpReqUserLoginField acc;
		memset(&acc, 0, sizeof(acc));
		strcpy(acc.LogInAccount, m_userid);
		acc.LogInAccountType = TORALEV2API::TORA_TSTP_LACT_UserID;
		strcpy(acc.Password, m_password);
		strcpy(acc.UserProductInfo ,"HX5ZJ0C1PV");
		m_api->ReqUserLogin(&acc, ++m_req_id);

	};

	virtual void Stop()
	{
		m_api->Release();
	}

	virtual void OnFrontDisconnected(int nReason)
	{
		m_logger->warn("Q, [OnFrontDisconnected], nReason:{}", nReason);
	};

	/// Error response
	virtual void OnRspError(TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspError!\n");
	};

	/// Login request response
	virtual void OnRspUserLogin(TORALEV2API::CTORATstpRspUserLoginField* pRspUserLogin, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID == 0)
		{
			m_logger->warn("Q, [OnRspUserLogin], Success!");
			
			//char* Securities[1];
			//Securities[0] = (char*) "300377";
			//int eid =  TORALEV2API::TORA_TSTP_EXD_SZSE;
			//Subscribe(Securities,1,eid);
		}

		else
		{
			m_logger->warn("Q, [OnRspUserLogin], Fail!");
		}
	};

    // Subscribe 
	virtual void Subscribe( char* ppSecurityID[], int nCount, TTORATstpExchangeIDType ExchageID)
	{
		//convert  array of char array into string
		std::string securities;
		for (int i = 0; i < nCount; ++i) {
			if (i > 0) securities += ", "; // Add a separator between IDs
			securities += ppSecurityID[i]; // Append the security ID to the string
		}

		if (ExchageID == TORALEV2API::TORA_TSTP_EXD_SSE) 
		{
			//Subscribe NGTS (orderdetial and trasaction together)
			int ret_nt = m_api->SubscribeNGTSTick(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
			if (ret_nt == 0)
			{
				m_logger->warn("Q, [SubscribeNGTSTick] , Success, stock:{}", securities);
			}
			else
			{
				m_logger->warn("Q, [SubscribeNGTSTick] , Fail,stock:{}", securities);
			}
		}
		
		
		//Subscribe old orderdetial 
		int ret_od = m_api->SubscribeOrderDetail(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_od == 0)
		{
			m_logger->warn("Q, [SubscribeOrderDetail] , Success,stock:{}", securities);
		}
		else
		{
			m_logger->warn("Q, [SubscribeOrderDetail] , Fail,stock:{}", securities);
		}

		//Subscribe old trasaction 
		int ret_t = m_api->SubscribeTransaction(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_t == 0)
		{
			int xtsmd_size = sizeof(ppSecurityID) / sizeof(char*);
			for (int i = 0; i < xtsmd_size; i++)
			{
				m_logger->warn("Q, [SubscribeTransaction] , Success,stock:{}", ppSecurityID[i]);
				//printf("SubscribeTransaction::Securities[%d]::%s\n", i, ppSecurityID[i]);
			}
		}
		else
		{
			m_logger->warn("Q, [SubscribeTransaction] , Failed");
		}

		//Subscribe to market data
		int ret_md = m_api->SubscribeMarketData(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_md == 0)
		{
			m_logger->warn("Q, [SubscribeMarketData] , Success,stock:{}", securities);
		}
		else
		{
			m_logger->warn("Q, [SubscribeMarketData] , Failed,stock:{}", securities);
		}
	}

    //UnSubscribe
	virtual void UnSubscribe(char* ppSecurityID[], int nCount, TTORATstpExchangeIDType ExchageID)
	{
		//convert  array of char array into string
		std::string securities;
		for (int i = 0; i < nCount; ++i) {
			if (i > 0) securities += ", "; // Add a separator between IDs
			securities += ppSecurityID[i]; // Append the security ID to the string
		}

		if (ExchageID == TORALEV2API::TORA_TSTP_EXD_SSE) 
		{
			//Subscribe NGTS (orderdetial and trasaction together)
			int ret_nt = m_api->UnSubscribeNGTSTick(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
			if (ret_nt == 0)
			{
				m_logger->warn("Q, [UnSubscribeNGTSTick] , Success,stock:{}", securities);
			}
			else
			{
				m_logger->warn("Q, [UnSubscribeNGTSTick] , Failed,stock:{}", securities);
			}
		}
		
		
		//Subscribe old orderdetial 
		int ret_od = m_api->UnSubscribeOrderDetail(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_od == 0)
		{
			m_logger->warn("Q, [UnSubscribeOrderDetail] , Success,stock:{}", securities);
		}
		else
		{
			m_logger->warn("Q, [UnSubscribeOrderDetail] , Failed,stock:{}", securities);
		}

		//Subscribe old trasaction 
		int ret_t = m_api->UnSubscribeTransaction(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_t == 0)
		{
			//printf("UnSubscribeTransaction:::Success,ret=%d\n", ret_t);
			//printf("UnSubscribeTransaction:::Success,eid=%d\n", ExchageID);
			int xtsmd_size = sizeof(ppSecurityID) / sizeof(char*);
			for (int i = 0; i < xtsmd_size; i++)
			{
				m_logger->warn("Q, [UnSubscribeTransaction] , Success,stock:{}", ppSecurityID[i]);
				//printf("UnSubscribeTransaction::Securities[%d]::%s\n", i, ppSecurityID[i]);
			}
		}
		else
		{
			m_logger->warn("Q, [UnSubscribeTransaction] , Failed,stock:{}", securities);
		}

		//Subscribe to market data
		int ret_md = m_api->UnSubscribeMarketData(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_md == 0)
		{
			m_logger->warn("Q, [UnSubscribeMarketData] , Success,stock:{}", securities);
		}
		else
		{
			m_logger->warn("Q, [UnSubscribeMarketData] , Failed,stock:{}", securities);
		}
	}



	/*********************************** Return Data Callback Function***********************************/
	/// Logout
	virtual void OnRspUserLogout(TORALEV2API::CTORATstpUserLogoutField* pUserLogout, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspUserLogout!\n");

	};

	/// Subscribe NGTSTICK
	virtual void OnRspSubNGTSTick(CTORATstpSpecificSecurityField* pSpecificSecurity, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID == 0 && pSpecificSecurity)
		{
			printf("OnRspSubNGTSTick SecurityID[%s] ExchangeID[%c] Success!\n", pSpecificSecurity->SecurityID, pSpecificSecurity->ExchangeID);

		}
	};

	/// OnRspSubMarketData
	virtual void OnRspSubMarketData(CTORATstpSpecificSecurityField* pSpecificSecurity, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID == 0 && pSpecificSecurity)
		{
			printf("OnRspSubMarketData SecurityID[%s] ExchangeID[%c] Success!\n", pSpecificSecurity->SecurityID, pSpecificSecurity->ExchangeID);

		}
	};

	/// OnRspUnSubMarketData
	virtual void OnRspUnSubMarketData(CTORATstpSpecificSecurityField* pSpecificSecurity, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspUnSubMarketData SecurityID[%s] ExchangeID[%c]!\n", pSpecificSecurity->SecurityID, pSpecificSecurity->ExchangeID);

	};


	// OnRspSubTransaction
	virtual void OnRspSubTransaction(CTORATstpSpecificSecurityField* pSpecificSecurity, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID == 0 && pSpecificSecurity)
		{
			printf("OnRspSubTransaction SecurityID[%s] ExchangeID[%c] Success!\n", pSpecificSecurity->SecurityID, pSpecificSecurity->ExchangeID);

		}
	};

	/// OnRspSubOrderDetail
	virtual void OnRspSubOrderDetail(CTORATstpSpecificSecurityField* pSpecificSecurity, TORALEV2API::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID == 0 && pSpecificSecurity)
		{
			printf("OnRspSubOrderDetail SecurityID[%s] ExchangeID[%c] Success!\n", pSpecificSecurity->SecurityID, pSpecificSecurity->ExchangeID);

		}
	};



	/*********************************** Data call back ***********************************/

	/// OnRtnMarketData
	virtual void OnRtnMarketData(CTORATstpLev2MarketDataField* pDepthMarketData, const int FirstLevelBuyNum, const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum, const int FirstLevelSellOrderVolumes[])
	{
		m_logger->info("Q, [OnRtnMarketData] ,{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
						pDepthMarketData->DataTimeStamp,
						pDepthMarketData->SecurityID,
						pDepthMarketData->ExchangeID,
						pDepthMarketData->PreClosePrice,
						pDepthMarketData->LowestPrice,
						pDepthMarketData->HighestPrice,
						pDepthMarketData->OpenPrice,
						pDepthMarketData->LastPrice,
						pDepthMarketData->BidPrice1, pDepthMarketData->BidPrice2, pDepthMarketData->BidPrice3,
						pDepthMarketData->BidPrice4, pDepthMarketData->BidPrice5, pDepthMarketData->BidPrice6,
						pDepthMarketData->BidPrice7, pDepthMarketData->BidPrice8, pDepthMarketData->BidPrice9, pDepthMarketData->BidPrice10,
						pDepthMarketData->AskPrice1, pDepthMarketData->AskPrice2, pDepthMarketData->AskPrice3,
						pDepthMarketData->AskPrice4, pDepthMarketData->AskPrice5, pDepthMarketData->AskPrice6,
						pDepthMarketData->AskPrice7, pDepthMarketData->AskPrice8, pDepthMarketData->AskPrice9, pDepthMarketData->AskPrice10,
						pDepthMarketData->BidVolume1, pDepthMarketData->BidVolume2, pDepthMarketData->BidVolume3,
						pDepthMarketData->BidVolume4, pDepthMarketData->BidVolume5, pDepthMarketData->BidVolume6,
						pDepthMarketData->BidVolume7, pDepthMarketData->BidVolume8, pDepthMarketData->BidVolume9, pDepthMarketData->BidVolume10,
						pDepthMarketData->AskVolume1, pDepthMarketData->AskVolume2, pDepthMarketData->AskVolume3,
						pDepthMarketData->AskVolume4, pDepthMarketData->AskVolume5, pDepthMarketData->AskVolume6,
						pDepthMarketData->AskVolume7, pDepthMarketData->AskVolume8, pDepthMarketData->AskVolume9, pDepthMarketData->AskVolume10);

		

		static_assert(sizeof(CTORATstpLev2MarketDataField) == sizeof(SE_Lev2MarketDataField), "Size mismatch");
		auto start = std::chrono::high_resolution_clock::now();
		SEEvent temp_event;
		std::shared_ptr<SE_Lev2MarketDataField> marketTick = SEObject::Create<SE_Lev2MarketDataField>();
		memcpy(marketTick.get(),pDepthMarketData,sizeof(SE_Lev2MarketDataField));

		temp_event.e_type = Eventtype::L2TICK;
		temp_event.event = marketTick;

		m_Event_Q_ptr->enqueue(std::move(temp_event));
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	
		/*
		for (int index = 0; index < FirstLevelBuyNum; index++)
		{
			printf("%d ", FirstLevelBuyOrderVolumes[index]);
		}
		printf("}");

		printf("SellVolumes %d{", FirstLevelSellNum);

		for (int index = 0; index < FirstLevelSellNum; index++)
		{
			printf("%d ", FirstLevelSellOrderVolumes[index]);
		}
		printf("}\n");
		*/
	};

	//NGT ֪ͨ
	virtual void OnRtnNGTSTick(CTORATstpLev2NGTSTickField* pTick)
	{
		m_logger->info("Q, [OnRtnNGTSTick] ,{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
						pTick->ExchangeID,
						pTick->SecurityID,
						pTick->MainSeq,
						pTick->SubSeq,
						pTick->TickTime,
						pTick->TickType,
						pTick->BuyNo,
						pTick->SellNo,
						pTick->Price,
						pTick->Volume,
						pTick->TradeMoney,
						pTick->Side,
						pTick->TradeBSFlag,
						pTick->MDSecurityStat,
						pTick->Info1,
						pTick->Info2,
						pTick->Info3);

		static_assert(sizeof(CTORATstpLev2NGTSTickField) == sizeof(SE_Lev2NGTSTickField), "Size mismatch");
		std::shared_ptr<SE_Lev2NGTSTickField> NGTSTick = SEObject::Create<SE_Lev2NGTSTickField>();
		memcpy(NGTSTick.get(),pTick,sizeof(SE_Lev2NGTSTickField));

		SEEvent temp_event;
		temp_event.e_type = Eventtype::NGTSTICK;
		temp_event.event = NGTSTick;
		m_Event_Q_ptr->enqueue(std::move(temp_event));


	}


	/// OnRtnTransaction
	virtual void OnRtnTransaction(CTORATstpLev2TransactionField* pTransaction)
	{
		m_logger->info("Q, [OnRtnTransaction] ,{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
						pTransaction->ExchangeID,
						pTransaction->SecurityID,
						pTransaction->TradeTime,
						pTransaction->TradePrice,
						pTransaction->TradeVolume,
						pTransaction->ExecType,
						pTransaction->MainSeq,
						pTransaction->SubSeq,
						pTransaction->BuyNo,
						pTransaction->SellNo,
						pTransaction->Info1,
						pTransaction->Info2,
						pTransaction->Info3,
						pTransaction->TradeBSFlag,
						pTransaction->BizIndex);

		static_assert(sizeof(CTORATstpLev2TransactionField) == sizeof(SE_Lev2TransactionStruct), "Size mismatch");
		auto start = std::chrono::high_resolution_clock::now();
		std::shared_ptr<SE_Lev2TransactionStruct> transstuct = SEObject::Create<SE_Lev2TransactionStruct>();
		memcpy(transstuct.get(),pTransaction,sizeof(SE_Lev2TransactionStruct));


		SEEvent temp_event;
		temp_event.e_type = Eventtype::TRANSACTION;
		temp_event.event = transstuct;
		m_Event_Q_ptr->enqueue(std::move(temp_event));
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
		
	};


	/// OnRtnOrderDetail
	virtual void OnRtnOrderDetail(CTORATstpLev2OrderDetailField* pOrderDetail)
	{
		m_logger->info("Q, [OnRtnOrderDetail] ,{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}",
						pOrderDetail->ExchangeID,
						pOrderDetail->SecurityID,
						pOrderDetail->OrderTime,
						pOrderDetail->Price,
						pOrderDetail->Volume,
						pOrderDetail->Side,
						pOrderDetail->OrderType,
						pOrderDetail->MainSeq,
						pOrderDetail->SubSeq,
						pOrderDetail->Info1,
						pOrderDetail->Info2,
						pOrderDetail->Info3,
						pOrderDetail->OrderNO,
						pOrderDetail->OrderStatus,
						pOrderDetail->BizIndex);

		static_assert(sizeof(CTORATstpLev2OrderDetailField) == sizeof(SE_Lev2OrderDetailField), "Size mismatch");
		auto start = std::chrono::high_resolution_clock::now();
		std::shared_ptr<SE_Lev2OrderDetailField> orderDetial = SEObject::Create<SE_Lev2OrderDetailField>();
		memcpy(orderDetial.get(),pOrderDetail,sizeof(SE_Lev2OrderDetailField));

		SEEvent temp_event;
		temp_event.e_type = Eventtype::ORDER_DETIAL;
		temp_event.event = orderDetial;
		m_Event_Q_ptr->enqueue(std::move(temp_event));
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	};

private:
	CTORATstpLev2MdApi* m_api = NULL;
	int m_req_id;
	char m_userid[21];
	char m_password[41];
	char m_address[64];
	char m_mode[21];

	moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr = nullptr;
	//moodycamel::ConcurrentQueue<std::shared_ptr<SEObject>>* m_Event_Q_ptr;
	LoggerPtr m_logger=nullptr;
	std::map<int,std::tuple<Strategy*,bool>>* m_StrategyMap_Ptr;

};