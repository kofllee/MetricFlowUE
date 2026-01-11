#pragma once

#include "MetricFlowEvent.h"
#include "MetricFlowHttpSender.h"
#include "MetricFlowSettings.h"
#include "MetricFlowSubsystem.generated.h"

struct FMetricFlowFields;

UCLASS()
class METRICFLOWCORE_API UMetricFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEvent(const FName& EventName, const FMetricFlowFields& ExtraContext, const FMetricFlowFields& Payload, const FString& SheetOverride);
	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEventMap(const FName& EventName, const TMap<FString, FString>& ExtraContextMap, const TMap<FString, FString>& PayloadMap, const FString& SheetOverride);

private:
	bool bEnabledRuntime = true;

	FString ProjectId;
	FString ProxyApiKey;
	
	FString ActiveEndpointURL;
	
	TArray<TObjectPtr<UMetricFlowContextProviderBase>> SessionContextProviders;
	TArray<TObjectPtr<UMetricFlowContextProviderBase>> EventContextProviders;

	TMap<FString, FString> CachedSessionContext;

	FString SessionId;

	TArray<FMetricFlowEvent> EventQueue;
	int32 MaxQueueSize = 2000;
	int32 BatchSize;

	float TimeoutSeconds;

	FTimerHandle TimerHandle;
	FMetricFlowHttpSender Sender = FMetricFlowHttpSender();

	bool bIsFlushing = false;
	TArray<FMetricFlowEvent> BatchEvents;

	void Flush();

	void LoadProvidersFromSettings(const UMetricFlowSettings* Settings);
	void RebuildSessionContextCache();
	
	static FMetricFlowEvent CreateMetricFlowEvent(const FName& EventName, const FMetricFlowFields& ExtraContext,
		const FMetricFlowFields& Payload, const FString& SheetOverride);
};
