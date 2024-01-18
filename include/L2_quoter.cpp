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
		std::cout<<"[Lev2MdSpi] creating m_api"<< std::endl;
		m_api = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
		std::cout<<"[Lev2MdSpi]  m_api created"<< std::endl;
		//m_api = api;
	}

	~Lev2MdSpi(void)
	{
	}

public:
	void init_queue(moodycamel::ConcurrentQueue<SEEvent>* m_Event_Q_ptr)
	{
		std::cout<< "[Lev2MdSpi] pointer before init :"<<m_Event_Q_ptr<<std::endl;
		this->m_Event_Q_ptr = m_Event_Q_ptr;
		std::cout<< "[Lev2MdSpi] pointer to queue inited, address is "<< m_Event_Q_ptr<<std::endl;
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
		m_api->RegisterFront(m_address);
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
		printf("OnFrontDisconnected! nReason[%d]\n", nReason);

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
			printf("OnRspUserLogin Success!\n");
			
			char* Securities[1];
			Securities[0] = (char*) "300377";
			int eid =  TORALEV2API::TORA_TSTP_EXD_SZSE;
			Subscribe(Securities,1,eid);
		}

		else
		{
			printf("OnRspUserLogin fail!\n");
		}
	};

    // Subscribe 
	virtual void Subscribe( char* ppSecurityID[], int nCount, TTORATstpExchangeIDType ExchageID)
	{
		if (ExchageID == TORALEV2API::TORA_TSTP_EXD_SSE) 
		{
			//Subscribe NGTS (orderdetial and trasaction together)
			int ret_nt = m_api->SubscribeNGTSTick(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
			if (ret_nt == 0)
			{
				printf("SubscribeNGTSTick:::Success,ret=%d\n", ret_nt);
			}
			else
			{
				printf("SubscribeNGTSTick:::Failed, ret=%d)\n", ret_nt);
			}
		}
		
		
		//Subscribe old orderdetial 
		int ret_od = m_api->SubscribeOrderDetail(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_od == 0)
		{
			printf("SubscribeOrderDetail:::Success,ret=%d\n", ret_od);
		}
		else
		{
			printf("SubscribeOrderDetail:::Failed, ret=%d)\n", ret_od);
		}

		//Subscribe old trasaction 
		int ret_t = m_api->SubscribeTransaction(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_t == 0)
		{
			printf("SubscribeTransaction:::Success,ret=%d\n", ret_t);
			printf("SubscribeTransaction:::Success,eid=%d\n", ExchageID);
			int xtsmd_size = sizeof(ppSecurityID) / sizeof(char*);
			for (int i = 0; i < xtsmd_size; i++)
			{
				printf("SubscribeTransaction::Securities[%d]::%s\n", i, ppSecurityID[i]);
			}
		}
		else
		{
			printf("SubscribeTransaction:::Failed,ret=%d)\n", ret_t);
		}

		//Subscribe to market data
		int ret_md = m_api->SubscribeMarketData(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_md == 0)
		{
			printf("SubscribeMarketData:::Success,ret=%d\n", ret_md);
		}
		else
		{
			printf("SubscribeMarketData:::Failed, ret=%d)\n", ret_md);
		}
	}

    //UnSubscribe
	virtual void UnSubscribe(char* ppSecurityID[], int nCount, TTORATstpExchangeIDType ExchageID)
	{
		if (ExchageID == TORALEV2API::TORA_TSTP_EXD_SSE) 
		{
			//Subscribe NGTS (orderdetial and trasaction together)
			int ret_nt = m_api->UnSubscribeNGTSTick(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
			if (ret_nt == 0)
			{
				printf("UnSubscribeNGTSTick:::Success,ret=%d\n", ret_nt);
			}
			else
			{
				printf("UnSubscribeNGTSTick:::Failed, ret=%d)\n", ret_nt);
			}
		}
		
		
		//Subscribe old orderdetial 
		int ret_od = m_api->UnSubscribeOrderDetail(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_od == 0)
		{
			printf("UnSubscribeOrderDetail:::Success,ret=%d\n", ret_od);
		}
		else
		{
			printf("UnSubscribeOrderDetail:::Failed, ret=%d)\n", ret_od);
		}

		//Subscribe old trasaction 
		int ret_t = m_api->UnSubscribeTransaction(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_t == 0)
		{
			printf("UnSubscribeTransaction:::Success,ret=%d\n", ret_t);
			printf("UnSubscribeTransaction:::Success,eid=%d\n", ExchageID);
			int xtsmd_size = sizeof(ppSecurityID) / sizeof(char*);
			for (int i = 0; i < xtsmd_size; i++)
			{
				printf("UnSubscribeTransaction::Securities[%d]::%s\n", i, ppSecurityID[i]);
			}
		}
		else
		{
			printf("UnSubscribeTransaction:::Failed,ret=%d)\n", ret_t);
		}

		//Subscribe to market data
		int ret_md = m_api->UnSubscribeMarketData(ppSecurityID, sizeof(ppSecurityID) / sizeof(char*), ExchageID);
		if (ret_md == 0)
		{
			printf("UnSubscribeMarketData:::Success,ret=%d\n", ret_md);
		}
		else
		{
			printf("UnSubscribeMarketData:::Failed, ret=%d)\n", ret_md);
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
		/*printf("OnRtnMarketData TimeStamp[%d]  SecurityID[%s] ExchangeID[%c]  PreClosePrice[%f] LowestPrice[%f] HighestPrice[%f] OpenPrice[%f] LastPrice[%f]"
			"BidPrice{[%f] [%f] [%f] [%f] [%f] [%f] [%f] [%f] [%f] [%f]}"
			"AskPrice{[%f] [%f] [%f] [%f] [%f] [%f] [%f] [%f] [%f] [%f]}"
			"BidVolume{[%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld]}"
			"AskVolume{[%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld] [%lld]}",
			pDepthMarketData->DataTimeStamp,
			pDepthMarketData->SecurityID,
			pDepthMarketData->ExchangeID, pDepthMarketData->PreClosePrice, pDepthMarketData->LowestPrice,
			pDepthMarketData->HighestPrice, pDepthMarketData->OpenPrice,
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
			pDepthMarketData->AskVolume7, pDepthMarketData->AskVolume8, pDepthMarketData->AskVolume9, pDepthMarketData->AskVolume10
		);*/

		

		static_assert(sizeof(CTORATstpLev2MarketDataField) == sizeof(SE_Lev2MarketDataField), "Size mismatch");
		auto start = std::chrono::high_resolution_clock::now();
		SEEvent temp_event;
		std::shared_ptr<SE_Lev2MarketDataField> marketTick = SEObject::Create<SE_Lev2MarketDataField>();
		memcpy(marketTick.get(),pDepthMarketData,sizeof(SE_Lev2MarketDataField));

		temp_event.e_type = Eventtype::L2TICK;
		temp_event.event = marketTick;

		//std::shared_ptr<SE_Lev2MarketDataField> temp = std::static_pointer_cast<SE_Lev2MarketDataField>(temp_event.event);
		//std::cout<<temp->SecurityID<<" "<<temp->ClosePrice<<std::endl;
		m_Event_Q_ptr->enqueue(std::move(temp_event));
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
		std::cout << "[OnRtnMarketData]: "
              << duration.count() << " nanoseconds" << std::endl;

	
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
		
		static_assert(sizeof(CTORATstpLev2NGTSTickField) == sizeof(SE_Lev2NGTSTickField), "Size mismatch");
		std::shared_ptr<SE_Lev2NGTSTickField> NGTSTick = SEObject::Create<SE_Lev2NGTSTickField>();
		memcpy(NGTSTick.get(),pTick,sizeof(SE_Lev2NGTSTickField));

		SEEvent temp_event;
		temp_event.e_type = Eventtype::NGTSTICK;
		temp_event.event = NGTSTick;
		m_Event_Q_ptr->enqueue(std::move(temp_event));


		printf("OnRtnNGTSTick SecurityID[%s] \n", pTick->SecurityID);
		printf("ExchangeID[%d] \n", pTick->ExchangeID);
		printf("Ticktime[%d] \n", pTick->TickTime);
		printf(" MainSeq[%d] \n", pTick->MainSeq);
		printf("SubSeq[%d] \n", pTick->SubSeq);
		printf("TickType[%c] \n", pTick->TickType);

		printf("BuyNo [%d] \n", pTick->BuyNo);
		printf("SellNo[%d] \n", pTick->SellNo);
		printf("Price[%.4f] \n", pTick->Price);
		printf(" Volume [%lld] \n", pTick->Volume);
		printf("TradeMoney[%.4f] \n", pTick->TradeMoney);
		printf("Side[%c] \n", pTick->Side);

		printf("TradeBSFlag [%c] \n", pTick->TradeBSFlag);
		printf("MDSecurityStat[%c] \n", pTick->MDSecurityStat);
		printf("Info1[%d] \n", pTick->Info1);
		printf(" Info2 [%d] \n", pTick->Info2);

	}


	/// OnRtnTransaction
	virtual void OnRtnTransaction(CTORATstpLev2TransactionField* pTransaction)
	{
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
		std::cout << "[OnRtnTransaction]: "
              << duration.count() << " nanoseconds" << std::endl;
		//printf("OnRtnTransaction SecurityID[%s] ", pTransaction->SecurityID);
		//printf("ExchangeID[%c] ", pTransaction->ExchangeID);
		//������ʳɽ���TradeTime�ĸ�ʽΪ��ʱ������롿��������??100221530����ʾ10:02:21.530;
		//�Ϻ���ʳɽ���TradeTime�ĸ�ʽΪ��ʱ����ٷ�֮�롿����??10022153����ʾ10:02:21.53;
		//printf("TradeTime[%d] ", pTransaction->TradeTime);
		/*
		printf("TradePrice[%.4f] ", pTransaction->TradePrice);
		printf("TradeVolume[%lld] ", pTransaction->TradeVolume);
		printf("ExecType[%c] ", pTransaction->ExecType);//�Ϻ���ʳɽ�û������ֶΣ�ֻ�������С�ֵ2��ʾ�����ɽ���BuyNo��SellNoֻ��һ���Ƿ�0ֵ���Ը÷�0���ȥ���ҵ������ί�м�Ϊ��������ί�С�
		printf("MainSeq[%d] ", pTransaction->MainSeq);
		printf("SubSeq[%lld] ", pTransaction->SubSeq);
		printf("BuyNo[%lld] ", pTransaction->BuyNo);
		printf("SellNo[%lld] ", pTransaction->SellNo);
		printf("TradeBSFlag[%c] ", pTransaction->TradeBSFlag);
		printf("Info1[%d] ", pTransaction->Info1);
		printf("Info2[%d] ", pTransaction->Info2);
		printf("Info3[%d] \n", pTransaction->Info3);
		*/
	};


	/// OnRtnOrderDetail
	virtual void OnRtnOrderDetail(CTORATstpLev2OrderDetailField* pOrderDetail)
	{
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
		std::cout << "[OnRtnOrderDetail]: "
              << duration.count() << " nanoseconds" << std::endl;
		
		//printf("OnRtnOrderDetail SecurityID[%s] \n", pOrderDetail->SecurityID);
		//printf("ExchangeID[%c] \n", pOrderDetail->ExchangeID);
		//printf("OrderTime[%d] \n", pOrderDetail->OrderTime);
		//printf("\n");

		//printf("Price[%.4f] \n", pOrderDetail->Price);
		//printf("Volume[%lld] \n", pOrderDetail->Volume);
		//printf("OrderType[%c] \n", pOrderDetail->OrderType);
		//printf("MainSeq[%d] \n", pOrderDetail->MainSeq);
		//printf("SubSeq[%d] \n", pOrderDetail->SubSeq);
		//printf("OrderNO[%lld] \n", pOrderDetail->OrderNO);
		//printf("OrderStatus[%c] \n", pOrderDetail->OrderStatus);
		//printf("Side[%c] \n", pOrderDetail->Side);
		//printf("Info1[%d] \n ", pOrderDetail->Info1);
		//printf("Info2[%d] \n", pOrderDetail->Info2);
		//printf("Info3[%d] \n", pOrderDetail->Info3);

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