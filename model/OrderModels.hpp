#pragma once
//#include "SEObject.hpp"
#include "TORATstpUserApiDataType.h"

class SEObject;

using namespace TORASTOCKAPI;

struct SE_CancelErrorField : public SEObject
{
    ///交易所代码
    TTORATstpExchangeIDType	ExchangeID;

    ///前置编号
    TTORATstpFrontIDType	FrontID;

    ///会话编号
    TTORATstpSessionIDType	SessionID;

    ///报单引用
    TTORATstpOrderRefType	OrderRef;

    ///系统报单编号
    TTORATstpOrderSysIDType	OrderSysID;

    ///委托方式
    TTORATstpOperwayType	Operway;

    ///字符串附加信息
    TTORATstpStrInfoType	SInfo;

    ///整形附加信息
    TTORATstpIntInfoType	IInfo;
};

struct SE_OrderErrorField : public SEObject
{
    ///交易所代码
    TTORATstpExchangeIDType ExchangeID;

    ///投资者代码
    TTORATstpInvestorIDType InvestorID;

    ///投资单元代码
    TTORATstpBusinessUnitIDType BusinessUnitID;

    ///股东账户代码
    TTORATstpShareholderIDType ShareholderID;

    ///证券代码
    TTORATstpSecurityIDType SecurityID;

    ///买卖方向
    TTORATstpDirectionType Direction;

    ///报单价格条件
    TTORATstpOrderPriceTypeType OrderPriceType;

    ///有效期类型
    TTORATstpTimeConditionType TimeCondition;

    ///成交量类型
    TTORATstpVolumeConditionType VolumeCondition;

    ///价格
    TTORATstpPriceType LimitPrice;

    ///数量
    TTORATstpVolumeType VolumeTotalOriginal;

    ///委托方式
    TTORATstpOperwayType Operway;

    ///报单引用
    TTORATstpOrderRefType OrderRef;

    ///港股通订单数量类型
    TTORATstpLotTypeType LotType;

    ///系统报单编号
    TTORATstpOrderSysIDType OrderSysID;

    ///条件检查
    TTORATstpCondCheckType CondCheck;

    ///有效日期
    TTORATstpDateType GTDate;

    ///强平原因(两融专用)
    TTORATstpForceCloseReasonType ForceCloseReason;

    ///指定偿还的信用负债编号（该字段置空表示不指定偿还）(两融专用)
    TTORATstpCreditDebtIDType CreditDebtID;

    ///头寸类型(两融专用)
    TTORATstpCreditQuotaTypeType CreditQuotaType;

    ///字符串附加信息
    TTORATstpStrInfoType SInfo;

    ///整形附加信息
    TTORATstpIntInfoType IInfo;
};

struct SE_InputOrderField : public SEObject
{
	///用户请求编号
	TTORATstpRequestIDType	UserRequestID;

	///交易所代码
	TTORATstpExchangeIDType	ExchangeID;

	///投资者代码
	TTORATstpInvestorIDType	InvestorID;

	///投资单元代码
	TTORATstpBusinessUnitIDType	BusinessUnitID;

	///股东账户代码
	TTORATstpShareholderIDType	ShareholderID;

	///证券代码
	TTORATstpSecurityIDType	SecurityID;

	///买卖方向
	TTORATstpDirectionType	Direction;

	///价格
	TTORATstpPriceType	LimitPrice;

	///数量
	TTORATstpVolumeType	VolumeTotalOriginal;

	///报单价格条件
	TTORATstpOrderPriceTypeType	OrderPriceType;

	///有效期类型
	TTORATstpTimeConditionType	TimeCondition;

	///成交量类型
	TTORATstpVolumeConditionType	VolumeCondition;

	///委托方式
	TTORATstpOperwayType	Operway;

	///报单引用
	TORASTOCKAPI::TTORATstpOrderRefType	OrderRef;

	///港股通订单数量类型
	TTORATstpLotTypeType	LotType;

	///系统报单编号
	TTORATstpOrderSysIDType	OrderSysID;

	///条件检查
	TTORATstpCondCheckType	CondCheck;

	///有效日期
	TTORATstpDateType	GTDate;

	///强平原因(两融专用)
	TTORATstpForceCloseReasonType	ForceCloseReason;

	///指定偿还的信用负债编号（该字段置空表示不指定偿还）(两融专用)
	TTORATstpCreditDebtIDType	CreditDebtID;

	///头寸类型(两融专用)
	TTORATstpCreditQuotaTypeType	CreditQuotaType;

	///费息折扣券编号（0表示不使用折扣券）(两融专用)
	TTORATstpIntSerialType	DiscountCouponID;

	///字符串附加信息
	TTORATstpStrInfoType	SInfo;

	///整形附加信息
	TTORATstpIntInfoType	IInfo;
};


struct SE_InputOrderActionField : public SEObject
	{
		///用户请求编号
		TTORATstpRequestIDType	UserRequestID;
	
		///交易所代码
		TTORATstpExchangeIDType	ExchangeID;
	
		///被撤报单前置编号
		TTORATstpFrontIDType	FrontID;
	
		///被撤报单会话编号
		TTORATstpSessionIDType	SessionID;
	
		///被撤报单引用
		TORASTOCKAPI::TTORATstpOrderRefType	OrderRef;
	
		///被撤报单系统编号
		TTORATstpOrderSysIDType	OrderSysID;
	
		///操作标志
		TTORATstpActionFlagType	ActionFlag;
	
		///报单操作引用
		TORASTOCKAPI::TTORATstpOrderRefType	OrderActionRef;
	
		///系统撤单编号
		TTORATstpOrderSysIDType	CancelOrderSysID;
	
		///委托方式
		TTORATstpOperwayType	Operway;
	
		///字符串附加信息
		TTORATstpStrInfoType	SInfo;
	
		///整形附加信息
		TTORATstpIntInfoType	IInfo;
	};


struct SE_TradeField : public SEObject
	{
		///交易所代码
		TTORATstpExchangeIDType	ExchangeID;
	
		///经纪公司部门代码
		TTORATstpDepartmentIDType	DepartmentID;
	
		///投资者代码
		TTORATstpInvestorIDType	InvestorID;
	
		///投资单元代码
		TTORATstpBusinessUnitIDType	BusinessUnitID;
	
		///股东账户代码
		TTORATstpShareholderIDType	ShareholderID;
	
		///证券代码
		TTORATstpSecurityIDType	SecurityID;
	
		///成交编号
		TTORATstpTradeIDType	TradeID;
	
		///买卖方向
		TTORATstpDirectionType	Direction;
	
		///系统报单编号
		TTORATstpOrderSysIDType	OrderSysID;
	
		///本地报单编号
		TTORATstpOrderLocalIDType	OrderLocalID;
	
		///成交价格
		TTORATstpPriceType	Price;
	
		///成交数量
		TTORATstpVolumeType	Volume;
	
		///成交日期
		TTORATstpDateType	TradeDate;
	
		///成交时间
		TTORATstpTimeType	TradeTime;
	
		///交易日
		TTORATstpDateType	TradingDay;
	
		///交易单元代码
		TTORATstpPbuIDType	PbuID;
	
		///报单引用
		TORASTOCKAPI::TTORATstpOrderRefType	OrderRef;
	
		///资金账户代码
		TTORATstpAccountIDType	AccountID;
	
		///币种
		TTORATstpCurrencyIDType	CurrencyID;
	};

struct SE_OrderField: public SEObject
	{
		///交易所代码
		TTORATstpExchangeIDType	ExchangeID;
	
		///投资者代码
		TTORATstpInvestorIDType	InvestorID;
	
		///投资单元代码
		TTORATstpBusinessUnitIDType	BusinessUnitID;
	
		///股东账户代码
		TTORATstpShareholderIDType	ShareholderID;
	
		///证券代码
		TTORATstpSecurityIDType	SecurityID;
	
		///买卖方向
		TTORATstpDirectionType	Direction;
	
		///报单价格条件
		TTORATstpOrderPriceTypeType	OrderPriceType;
	
		///有效期类型
		TTORATstpTimeConditionType	TimeCondition;
	
		///成交量类型
		TTORATstpVolumeConditionType	VolumeCondition;
	
		///价格
		TTORATstpPriceType	LimitPrice;
	
		///数量
		TTORATstpVolumeType	VolumeTotalOriginal;
	
		///港股通订单数量类型
		TTORATstpLotTypeType	LotType;
	
		///有效日期
		TTORATstpDateType	GTDate;
	
		///委托方式
		TTORATstpOperwayType	Operway;
	
		///条件检查
		TTORATstpCondCheckType	CondCheck;
	
		///字符串附加信息
		TTORATstpStrInfoType	SInfo;
	
		///整形附加信息
		TTORATstpIntInfoType	IInfo;
	
		///请求编号
		TTORATstpRequestIDType	RequestID;
	
		///前置编号
		TTORATstpFrontIDType	FrontID;
	
		///会话编号
		TTORATstpSessionIDType	SessionID;
	
		///报单引用
		TORASTOCKAPI::TTORATstpOrderRefType	OrderRef;
	
		///本地报单编号
		TTORATstpOrderLocalIDType	OrderLocalID;
	
		///系统报单编号
		TTORATstpOrderSysIDType	OrderSysID;
	
		///报单状态
		TTORATstpOrderStatusType	OrderStatus;
	
		///报单提交状态
		TTORATstpOrderSubmitStatusType	OrderSubmitStatus;
	
		///状态信息
		TTORATstpErrorMsgType	StatusMsg;
	
		///已成交数量
		TTORATstpVolumeType	VolumeTraded;
	
		///已撤销数量
		TTORATstpVolumeType	VolumeCanceled;
	
		///交易日
		TTORATstpDateType	TradingDay;
	
		///申报用户
		TTORATstpUserIDType	InsertUser;
	
		///申报日期
		TTORATstpDateType	InsertDate;
	
		///申报时间
		TTORATstpTimeType	InsertTime;
	
		///交易所接收时间
		TTORATstpTimeType	AcceptTime;
	
		///撤销用户
		TTORATstpUserIDType	CancelUser;
	
		///撤销时间
		TTORATstpTimeType	CancelTime;
	
		///经纪公司部门代码
		TTORATstpDepartmentIDType	DepartmentID;
	
		///资金账户代码
		TTORATstpAccountIDType	AccountID;
	
		///币种
		TTORATstpCurrencyIDType	CurrencyID;
	
		///交易单元代码
		TTORATstpPbuIDType	PbuID;
	
		///成交金额
		TTORATstpMoneyType	Turnover;
	
		///报单类型
		TTORATstpOrderTypeType	OrderType;
	
		///用户端产品信息
		TTORATstpUserProductInfoType	UserProductInfo;
	
		///强平原因(两融专用)
		TTORATstpForceCloseReasonType	ForceCloseReason;
	
		///信用头寸编号(两融专用)
		TTORATstpQuotaIDType	CreditQuotaID;
	
		///头寸类型(两融专用)
		TTORATstpCreditQuotaTypeType	CreditQuotaType;
	
		///信用负债编号(两融专用)
		TTORATstpCreditDebtIDType	CreditDebtID;
	
		///IP地址
		TTORATstpIPAddressType	IPAddress;
	
		///Mac地址
		TTORATstpMacAddressType	MacAddress;
	
		///回报附加浮点型数据信息
		TTORATstpFloatInfoType	RtnFloatInfo;
	
		///回报附加整型数据
		TTORATstpIntInfoType	RtnIntInfo;
	
		///回报附加浮点型数据1
		TTORATstpFloatInfoType	RtnFloatInfo1;
	
		///回报附加浮点型数据2
		TTORATstpFloatInfoType	RtnFloatInfo2;
	
		///回报附加浮点型数据3
		TTORATstpFloatInfoType	RtnFloatInfo3;
	};

