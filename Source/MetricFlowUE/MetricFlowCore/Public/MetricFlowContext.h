#pragma once

#include "MetricFlowContext.generated.h"

USTRUCT(BlueprintType)
struct FMetricFlowContext
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metric Flow")
	FString SessionId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metric Flow")
	FString UserId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metric Flow")
	FString BuildVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metric Flow")
	FString Platform;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metric Flow")
	FString PlatformVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metric Flow")
	FString LevelName;

public:
	FORCEINLINE bool HasSessionId() const { return !SessionId.IsEmpty(); }
};
