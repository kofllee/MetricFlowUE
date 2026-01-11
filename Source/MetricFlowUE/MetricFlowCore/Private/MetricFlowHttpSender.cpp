#include "MetricFlowHttpSender.h"

#include "HttpModule.h"
#include "MetricFlowLog.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void FMetricFlowHttpSender::PostJson(const FString& Url, const FString& ProxyApiKey, const FString& JsonBody, const float TimeoutSeconds,
                                     TFunction<void(bool, int32, const FString&)> OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!ProxyApiKey.IsEmpty())
		HttpRequest->SetHeader(TEXT("x-api-key"), ProxyApiKey);
	HttpRequest->SetContentAsString(JsonBody);

	HttpRequest->OnProcessRequestComplete().BindLambda(
	    [OnComplete](FHttpRequestPtr, const FHttpResponsePtr& Response, bool bConnected)
	    {
		    int32 ResponseCode = Response.IsValid() ? Response->GetResponseCode() : -1;
		    FString ResponseBody = Response.IsValid() ? Response->GetContentAsString() : FString();

	    	const bool bWasSuccessful = bConnected && Response.IsValid() && (ResponseCode >= 200 && ResponseCode < 300);
		    OnComplete(bWasSuccessful, ResponseCode, ResponseBody);
	    });

	HttpRequest->SetTimeout(TimeoutSeconds);
	HttpRequest->ProcessRequest();

	UE_LOG(LogTemp, Display, TEXT("MetricFlowHttpSender::PostJson: Sent POST request to %s"), *Url);
}
