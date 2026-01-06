#pragma once

#include "MetricFlowLog.h"
#include "MetricFlowPayload.generated.h"

USTRUCT(BlueprintType)
struct METRICFLOWCORE_API FMetricFlowPayload
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	TMap<FString, FString> Data;

public:
	FORCEINLINE bool IsEmpty() const { return Data.Num() == 0; }
	FORCEINLINE int32 Num() const { return Data.Num(); }

	FORCEINLINE FMetricFlowPayload& Add(const FString& Key, const FString& Value)
	{
		if (!SetString(Key, Value))
		{
			UE_LOG(LogMetricFlow, Error, TEXT("MetricFlowPayload::Add - Failed to add key-value pair. Key is empty."));
		}
		return *this;
	}

	FORCEINLINE FMetricFlowPayload& AddInt(const FString& Key, const int32 Value)
	{
		return Add(Key, FString::FromInt(Value));
	}
	FORCEINLINE FMetricFlowPayload& AddFloat(const FString& Key, const float Value)
	{
		return Add(Key, FString::SanitizeFloat(Value));
	}
	FORCEINLINE FMetricFlowPayload& AddBool(const FString& Key, const bool Value)
	{
		return Add(Key, Value ? TEXT("true") : TEXT("false"));
	}

private:
	bool SetString(const FString& Key, const FString& Value)
	{
		if (Key.IsEmpty())
			return false;
		
		Data.Add(Key, Value);
		return true;
	}
};
