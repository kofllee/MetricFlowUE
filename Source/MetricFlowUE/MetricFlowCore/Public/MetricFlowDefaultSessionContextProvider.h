#pragma once

#include "MetricFlowSessionContextProviderBase.h"
#include "MetricFlowDefaultSessionContextProvider.generated.h"

UCLASS(Blueprintable, BlueprintType, DefaultToInstanced)
class METRICFLOWCORE_API UMetricFlowDefaultSessionContextProvider : public UMetricFlowSessionContextProviderBase
{
	GENERATED_BODY()
public:
	virtual void CollectFields_Implementation(UObject* WorldContextObject, TMap<FString, FString>& OutContext) const override;
};
