#pragma once
//#include "SEObject.hpp"
#include "TORATstpLev2ApiDataType.h"

class SEObject;

using namespace TORALEV2API;

struct SE_Lev2TransactionStruct : public SEObject
	{
			
		///交易所代码
		TTORATstpExchangeIDType	ExchangeID;

		///证券代码
		TTORATstpSecurityIDType	SecurityID;

		///时间戳
		TTORATstpTimeStampType	TradeTime;

		///成交价格
		TTORATstpPriceType	TradePrice;

		///成交数量
		TTORATstpLongVolumeType	TradeVolume;

		///成交类别（只有深圳行情有效）
		TTORATstpExecTypeType	ExecType;

		///主序号
		TTORATstpSequenceNoType	MainSeq;

		///子序号
		TTORATstpLongSequenceType	SubSeq;

		///买方委托序号
		TTORATstpLongSequenceType	BuyNo;

		///卖方委托序号
		TTORATstpLongSequenceType	SellNo;

		///附加信息1
		TTORATstpIntInfoType	Info1;

		///附加信息2
		TTORATstpIntInfoType	Info2;

		///附加信息3
		uint64_t 	Info3;

		///内外盘标志（只有上海行情有效）
		TTORATstpTradeBSFlagType	TradeBSFlag;

		///业务序号（只有上海行情有效）
		TTORATstpLongSequenceType	BizIndex;

	};

struct SE_Lev2OrderDetailField : public SEObject
	{
			
		///交易所代码
		TTORATstpExchangeIDType	ExchangeID;

		///证券代码
		TTORATstpSecurityIDType	SecurityID;

		///时间戳
		TTORATstpTimeStampType	OrderTime;

		///委托价格
		TTORATstpPriceType	Price;

		///委托数量
		TTORATstpLongVolumeType	Volume;

		///委托方向
		TTORATstpLSideType	Side;

		///订单类别（只有深圳行情有效）
		TTORATstpLOrderTypeType	OrderType;

		///主序号
		TTORATstpSequenceNoType	MainSeq;

		///子序号
		TTORATstpSequenceNoType	SubSeq;

		///附加信息1
		TTORATstpIntInfoType	Info1;

		///附加信息2
		TTORATstpIntInfoType	Info2;

		///附加信息3
		uint64_t 	Info3;

		///委托序号
		TTORATstpLongSequenceType	OrderNO;

		///订单状态
		TTORATstpLOrderStatusType	OrderStatus;

		///业务序号（只有上海行情有效）
		TTORATstpLongSequenceType	BizIndex;

	};

struct SE_Lev2NGTSTickField : public SEObject
	{
			
		///交易所代码
		TTORATstpExchangeIDType	ExchangeID;

		///证券代码
		TTORATstpSecurityIDType	SecurityID;

		///主序号
		TTORATstpSequenceNoType	MainSeq;

		///子序号
		TTORATstpLongSequenceType	SubSeq;

		///时间戳
		TTORATstpTimeStampType	TickTime;

		///逐笔类型
		TTORATstpLTickTypeType	TickType;

		///买方委托序号
		TTORATstpLongSequenceType	BuyNo;

		///卖方委托序号
		TTORATstpLongSequenceType	SellNo;

		///价格
		TTORATstpPriceType	Price;

		///数量
		TTORATstpLongVolumeType	Volume;

		///成交金额或新增委托订单已成交委托量
		TTORATstpMoneyType	TradeMoney;

		///委托方向
		TTORATstpLSideType	Side;

		///内外盘标志
		TTORATstpTradeBSFlagType	TradeBSFlag;

		///行情产品实时状态
		TTORATstpMDSecurityStatType	MDSecurityStat;

		///附加信息1
		TTORATstpIntInfoType	Info1;

		///附加信息2
		TTORATstpIntInfoType	Info2;

		///附加信息3
		TTORATstpIntInfoType	Info3;

	};

struct SE_Lev2MarketDataField : public SEObject
	{
			
		///证券代码
		TTORATstpSecurityIDType	SecurityID;

		///行情交易所代码
		TTORATstpExchangeIDType	ExchangeID;

		///时间戳
		TTORATstpTimeStampType	DataTimeStamp;

		///昨收盘
		TTORATstpPriceType	PreClosePrice;

		///今开盘
		TTORATstpPriceType	OpenPrice;

		///成交笔数
		TTORATstpLongVolumeType	NumTrades;

		///成交总量
		TTORATstpLongVolumeType	TotalVolumeTrade;

		///成交总金额
		TTORATstpMoneyType	TotalValueTrade;

		///委托买入总量
		TTORATstpLongVolumeType	TotalBidVolume;

		///加权平均委托买价格
		TTORATstpPriceType	AvgBidPrice;

		///委托卖出总量
		TTORATstpLongVolumeType	TotalAskVolume;

		///加权平均委托卖价格
		TTORATstpPriceType	AvgAskPrice;

		///最高价
		TTORATstpPriceType	HighestPrice;

		///最低价
		TTORATstpPriceType	LowestPrice;

		///现价
		TTORATstpPriceType	LastPrice;

		///申买价一
		TTORATstpPriceType	BidPrice1;

		///申买量一
		TTORATstpLongVolumeType	BidVolume1;

		///申卖价一
		TTORATstpPriceType	AskPrice1;

		///申卖量一
		TTORATstpLongVolumeType	AskVolume1;

		///申卖价二
		TTORATstpPriceType	AskPrice2;

		///申卖量二
		TTORATstpLongVolumeType	AskVolume2;

		///申卖价三
		TTORATstpPriceType	AskPrice3;

		///申卖量三
		TTORATstpLongVolumeType	AskVolume3;

		///申买价二
		TTORATstpPriceType	BidPrice2;

		///申买量二
		TTORATstpLongVolumeType	BidVolume2;

		///申买价三
		TTORATstpPriceType	BidPrice3;

		///申买量三
		TTORATstpLongVolumeType	BidVolume3;

		///申卖价四
		TTORATstpPriceType	AskPrice4;

		///申卖量四
		TTORATstpLongVolumeType	AskVolume4;

		///申卖价五
		TTORATstpPriceType	AskPrice5;

		///申卖量五
		TTORATstpLongVolumeType	AskVolume5;

		///申买价四
		TTORATstpPriceType	BidPrice4;

		///申买量四
		TTORATstpLongVolumeType	BidVolume4;

		///申买价五
		TTORATstpPriceType	BidPrice5;

		///申买量五
		TTORATstpLongVolumeType	BidVolume5;

		///申卖价六
		TTORATstpPriceType	AskPrice6;

		///申卖量六
		TTORATstpLongVolumeType	AskVolume6;

		///申卖价七
		TTORATstpPriceType	AskPrice7;

		///申卖量七
		TTORATstpLongVolumeType	AskVolume7;

		///申买价六
		TTORATstpPriceType	BidPrice6;

		///申买量六
		TTORATstpLongVolumeType	BidVolume6;

		///申买价七
		TTORATstpPriceType	BidPrice7;

		///申买量七
		TTORATstpLongVolumeType	BidVolume7;

		///申卖价八
		TTORATstpPriceType	AskPrice8;

		///申卖量八
		TTORATstpLongVolumeType	AskVolume8;

		///申卖价九
		TTORATstpPriceType	AskPrice9;

		///申卖量九
		TTORATstpLongVolumeType	AskVolume9;

		///申买价八
		TTORATstpPriceType	BidPrice8;

		///申买量八
		TTORATstpLongVolumeType	BidVolume8;

		///申买价九
		TTORATstpPriceType	BidPrice9;

		///申买量九
		TTORATstpLongVolumeType	BidVolume9;

		///申买价十
		TTORATstpPriceType	BidPrice10;

		///申买量十
		TTORATstpLongVolumeType	BidVolume10;

		///申卖价十
		TTORATstpPriceType	AskPrice10;

		///申卖量十
		TTORATstpLongVolumeType	AskVolume10;

		///附加信息1
		TTORATstpIntInfoType	Info1;

		///附加信息2
		TTORATstpIntInfoType	Info2;

		///附加信息3
		TTORATstpIntInfoType	Info3;

		///涨停板价(只有深圳行情有效)
		TTORATstpPriceType	UpperLimitPrice;

		///跌停板价(只有深圳行情有效)
		TTORATstpPriceType	LowerLimitPrice;

		///今收盘价(只有上海行情有效)
		TTORATstpPriceType	ClosePrice;

		///行情产品实时状态
		TTORATstpMDSecurityStatType	MDSecurityStat;

		///买入总笔数(只有上海行情有效)
		TTORATstpVolumeType	TotalBidNumber;

		///卖出总笔数(只有上海行情有效)
		TTORATstpVolumeType	TotalOfferNumber;

		///买入委托成交最大等待时间(只有上海行情有效)
		TTORATstpVolumeType	BidTradeMaxDuration;

		///卖出委托成交最大等待时间(只有上海行情有效)
		TTORATstpVolumeType	OfferTradeMaxDuration;

		///基金实时参考净值
		TTORATstpPriceType	IOPV;

		///卖一价上总委托笔数
		TTORATstpVolumeType	Ask1NumOrders;

		///买一价上总委托笔数
		TTORATstpVolumeType	Bid1NumOrders;

		///卖二价上总委托笔数
		TTORATstpVolumeType	Ask2NumOrders;

		///买二价上总委托笔数
		TTORATstpVolumeType	Bid2NumOrders;

		///卖三价上总委托笔数
		TTORATstpVolumeType	Ask3NumOrders;

		///买三价上总委托笔数
		TTORATstpVolumeType	Bid3NumOrders;

		///卖四价上总委托笔数
		TTORATstpVolumeType	Ask4NumOrders;

		///买四价上总委托笔数
		TTORATstpVolumeType	Bid4NumOrders;

		///卖五价上总委托笔数
		TTORATstpVolumeType	Ask5NumOrders;

		///买五价上总委托笔数
		TTORATstpVolumeType	Bid5NumOrders;

		///卖六价上总委托笔数
		TTORATstpVolumeType	Ask6NumOrders;

		///买六价上总委托笔数
		TTORATstpVolumeType	Bid6NumOrders;

		///卖七价上总委托笔数
		TTORATstpVolumeType	Ask7NumOrders;

		///买七价上总委托笔数
		TTORATstpVolumeType	Bid7NumOrders;

		///卖八价上总委托笔数
		TTORATstpVolumeType	Ask8NumOrders;

		///买八价上总委托笔数
		TTORATstpVolumeType	Bid8NumOrders;

		///卖九价上总委托笔数
		TTORATstpVolumeType	Ask9NumOrders;

		///买九价上总委托笔数
		TTORATstpVolumeType	Bid9NumOrders;

		///卖十价上总委托笔数
		TTORATstpVolumeType	Ask10NumOrders;

		///买十价上总委托笔数
		TTORATstpVolumeType	Bid10NumOrders;

		///买入撤单笔数(只有上海行情有效)
		TTORATstpVolumeType	WithdrawBuyNumber;

		///买入撤单数量(只有上海行情有效)
		TTORATstpLongVolumeType	WithdrawBuyAmount;

		///买入撤单金额(只有上海行情有效)
		TTORATstpMoneyType	WithdrawBuyMoney;

		///卖出撤单笔数(只有上海行情有效)
		TTORATstpVolumeType	WithdrawSellNumber;

		///卖出撤单数量(只有上海行情有效)
		TTORATstpLongVolumeType	WithdrawSellAmount;

		///卖出撤单金额(只有上海行情有效)
		TTORATstpMoneyType	WithdrawSellMoney;

	};