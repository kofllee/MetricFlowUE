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
	bool IsEmpty() const { return Data.Num() == 0; }
	int32 Num() const { return Data.Num(); }

	FMetricFlowPayload& AddString(const FString& Key, const FString& Value)
	{
		if (!SetString(Key, Value))
		{
			UE_LOG(LogMetricFlow, Error, TEXT("MetricFlowPayload::Add - Failed to add key-value pair. Key is empty."));
		}
		return *this;
	}
	
	FMetricFlowPayload& AddInt(const FString& Key, const int32 Value)
	{
		return AddString(Key, FString::FromInt(Value));
	}
	FMetricFlowPayload& AddFloat(const FString& Key, const float Value)
	{
		return AddString(Key, FString::SanitizeFloat(Value));
	}
	FMetricFlowPayload& AddBool(const FString& Key, const bool Value)
	{
		return AddString(Key, Value ? TEXT("true") : TEXT("false"));
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
