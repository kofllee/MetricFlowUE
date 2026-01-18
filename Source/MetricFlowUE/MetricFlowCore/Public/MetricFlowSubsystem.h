#pragma once

#include "MetricFlowEvent.h"
#include "MetricFlowHttpSender.h"
#include "MetricFlowSettings.h"
#include "MetricFlowSubsystem.generated.h"

enum class EMetricFlowOp : uint8
{
	UpsertSession,
	AppendEvents,
};

enum class EMetricFlowInFlightPolicy : uint8
{
	WaitUntilIdle,
	Ignore
};

FORCEINLINE const TCHAR* ToString(EMetricFlowOp Op)
{
	switch (Op)
	{
	case EMetricFlowOp::UpsertSession: return TEXT("UpsertSession");
	case EMetricFlowOp::AppendEvents:  return TEXT("AppendEvents");
	default:                           return TEXT("Unknown");
	}
}

struct FMetricFlowPendingRequest
{
	EMetricFlowOp Op;
	EMetricFlowInFlightPolicy InFlightPolicy = EMetricFlowInFlightPolicy::WaitUntilIdle;
	FString Json;

	TArray<FMetricFlowEvent> SendBatch;
};


struct FMetricFlowFields;

UCLASS()
class METRICFLOWCORE_API UMetricFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEvent(const FName& EventName, const FMetricFlowFields& ExtraContext, const FMetricFlowFields& Payload, const TArray<FString>& ExtraSheets);
	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEventMap(const FName& EventName, const TMap<FString, FString>& ExtraContextMap, const TMap<FString, FString>& PayloadMap, const TArray<FString>& ExtraSheets);

private:
	bool bEnabledRuntime = true;

	FString ProjectId;
	FString ProxyApiKey;
	
	FString ActiveEndpointURL;

	UPROPERTY()
	TArray<TObjectPtr<UMetricFlowContextProviderBase>> SessionContextProviders;
	UPROPERTY()
	TArray<TObjectPtr<UMetricFlowContextProviderBase>> EventContextProviders;

	TMap<FString, FString> CachedSessionContext;

	FString SessionId;
	FString SessionStartedAtUTC;
	FString SessionEndedAtUTC;

	TArray<FMetricFlowEvent> EventQueue;
	int64 EventSeq = 0;
	int32 MaxQueueSize = 2000;
	int32 MaxShutdownBatches;
	int32 MaxBatchSize;
	int32 MinBatchSize;
	int32 MaxLastBatchSize;

	float ShutdownFlushTimeMs;
	bool bShutdownWaitForResponses;

	float TimeoutSeconds;

	EMetricFlowShutdownMode ShutdownMode = EMetricFlowShutdownMode::FlushThenPersist;

	FTimerHandle TimerHandle;
	FMetricFlowHttpSender Sender = FMetricFlowHttpSender();
	
	uint32 RequestsInFlight = 0;

	bool BuildUpsertSessionRequest(FMetricFlowPendingRequest& OutReq);
	bool BuildAppendEventsRequest(FMetricFlowPendingRequest& OutReq, const int32 MinSendBatchSize, const int32 MaxSendBatchSize);
	
	void TrySendUpsertSession();
	void TrySendAppendEvents();
	bool SendRequest(FMetricFlowPendingRequest Req);
	void OnRequestCompleted(const FMetricFlowPendingRequest& Req, const bool bWasSuccessful, const int32 ResponseCode, const FString& ResponseBody);

	void TrySendLastEvents(const double EndTime);
	void PersistQueueToDisk();
	
	void LoadProvidersFromSettings(const UMetricFlowSettings* Settings);
	void RebuildSessionContextCache();
	
	static FMetricFlowEvent CreateMetricFlowEvent(const FName& EventName, const int64 Seq, const FMetricFlowFields& ExtraContext,
		const FMetricFlowFields& Payload, const TArray<FString>& ExtraSheets);
};
