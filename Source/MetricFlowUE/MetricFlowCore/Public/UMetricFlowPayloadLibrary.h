#pragma once

#include "MetricFlowPayload.h"
#include "UMetricFlowPayloadLibrary.generated.h"

UCLASS()
class METRICFLOWCORE_API UMetricFlowPayloadLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowPayload CreateMetricFlowPayload();

	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowPayload AddString(FMetricFlowPayload Payload, const FString& Key, const FString& Value);
	
	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowPayload AddInt(FMetricFlowPayload Payload, const FString& Key, const int32 Value);

	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowPayload AddFloat(FMetricFlowPayload Payload, const FString& Key, const float Value);
	
	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowPayload AddBool(FMetricFlowPayload Payload, const FString& Key, const bool Value);
};
