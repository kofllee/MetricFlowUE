#pragma once
#include "MetricFlowFields.h"
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
	int64 SequenceNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Metric Flow")
	FMetricFlowFields Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	FMetricFlowFields Payload;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	FString SheetOverride;
};
