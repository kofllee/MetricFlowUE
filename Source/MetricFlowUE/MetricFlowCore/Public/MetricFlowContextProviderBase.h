#pragma once

#include "MetricFlowContextProviderBase.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UMetricFlowContextProviderBase : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Metric Flow|Context")
	void CollectFields(UObject* WorldContextObject, TMap<FString, FString>& OutFields) const;

	virtual void CollectFields_Implementation(UObject* WorldContextObject, TMap<FString, FString>& OutFields) const;
};
