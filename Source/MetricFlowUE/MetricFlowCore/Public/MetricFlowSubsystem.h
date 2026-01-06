#pragma once

#include "MetricFlowContext.h"
#include "MetricFlowEvent.h"
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
	void RecordEventPayload(const FName& EventName, const FMetricFlowPayload& Payload, const FString& SheetOverride);
	UFUNCTION(BlueprintCallable, Category="Metric Flow")
	void RecordEventMap(const FName& EventName, const TMap<FString, FString>& Map, const FString& SheetOverride);

private:
	bool bEnabledRuntime = true;

	FString ActiveEndpointURL;
	FString ActiveDefaultSheet;
	
	FMetricFlowContext Context;

	int64 SequenceCounter = 0;

	FTimerHandle TimerHandle;

	void TickFlush();
	
	static FMetricFlowPayload CreatePayloadFromMap(const TMap<FString, FString>& Map);
	static FMetricFlowEvent CreateMetricFlowEvent(const FName& EventName, const FMetricFlowPayload& Payload,
		const FString& SheetOverride);
};
