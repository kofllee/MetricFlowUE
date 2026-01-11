#pragma once

#include "MetricFlowFields.h"
#include "UMetricFlowFieldsLibrary.generated.h"

UCLASS()
class METRICFLOWCORE_API UMetricFlowFieldsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields CreateMetricFlowFields();

	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields AddString(FMetricFlowFields Fields, const FString& Key, const FString& Value);
	
	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields AddInt(FMetricFlowFields Fields, const FString& Key, const int32 Value);

	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields AddFloat(FMetricFlowFields Fields, const FString& Key, const float Value);
	
	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields AddBool(FMetricFlowFields Fields, const FString& Key, const bool Value);

	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields AddMap(FMetricFlowFields Fields, const TMap<FString, FString>& Values);

	UFUNCTION(BlueprintPure, Category="Metric Flow")
	static FMetricFlowFields CombineFields(FMetricFlowFields A, const FMetricFlowFields& B);
};
