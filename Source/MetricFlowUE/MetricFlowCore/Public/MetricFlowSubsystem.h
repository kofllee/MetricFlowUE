#pragma once

#include "MetricFlowContext.h"
#include "MetricFlowEvent.h"
#include "MetricFlowHttpSender.h"
#include "MetricFlowSettings.h"
#include "MetricFlowSubsystem.generated.h"

struct FMetricFlowPayload;

UCLASS()
class METRICFLOWCORE_API UMetricFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEvent(const FName& EventName, const FMetricFlowPayload& Payload, const FString& SheetOverride);
	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEventMap(const FName& EventName, const TMap<FString, FString>& Map, const FString& SheetOverride);

private:
	bool bEnabledRuntime = true;

	FString ProjectToken;
	
	FString ActiveEndpointURL;
	FString ActiveDefaultSheet;
	
	FMetricFlowContext Context;

	TArray<FMetricFlowEvent> EventQueue;
	int32 MaxQueueSize = 2000;
	int32 BatchSize;

	float TimeoutSeconds;

	FTimerHandle TimerHandle;
	FMetricFlowHttpSender Sender = FMetricFlowHttpSender();

	bool bIsFlushing = false;
	TArray<FMetricFlowEvent> BatchEvents;

	void Flush();
	
	static FMetricFlowPayload CreatePayloadFromMap(const TMap<FString, FString>& Map);
	static FMetricFlowEvent CreateMetricFlowEvent(const FName& EventName, const FMetricFlowPayload& Payload,
		const FString& SheetOverride);
};
