#pragma once

class IHttpRequest;

class FMetricFlowHttpSender
{
public:
	void PostJson(const FString& Url, const FString& ProxyApiKey, const FString& JsonBody, const float TimeoutSeconds, TFunction<void(bool, int32, const FString&)> Callback);
};
