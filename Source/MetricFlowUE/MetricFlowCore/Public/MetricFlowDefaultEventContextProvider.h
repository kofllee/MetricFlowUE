#pragma once

#include "MetricFlowEventContextProviderBase.h"
#include "MetricFlowDefaultEventContextProvider.generated.h"

UCLASS(Blueprintable, BlueprintType, DefaultToInstanced)
class METRICFLOWCORE_API UMetricFlowDefaultEventContextProvider : public UMetricFlowEventContextProviderBase
{
	GENERATED_BODY()
public:
	virtual void CollectFields_Implementation(UObject* WorldContextObject, TMap<FString, FString>& OutContext) const override;
};
