#pragma once

class FMetricFlowHttpSender
{
public:
	void PostJson(const FString& Url, const FString& JsonBody, TFunction<void(bool, int32, const FString&)> Callback);
};
