#pragma once
#include "MetricFlowPayload.h"
#include "MetricFlowEvent.generated.h"

USTRUCT(BlueprintType)
struct METRICFLOWCORE_API FMetricFlowEvent
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	FName EventName;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Metric Flow")
	FDateTime TimestampUTC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Metric Flow")
	FMetricFlowContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	FMetricFlowPayload Payload;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	FString SheetOverride;
};
