#pragma once

#include "MetricFlowLog.h"
#include "MetricFlowFields.generated.h"

USTRUCT(BlueprintType)
struct METRICFLOWCORE_API FMetricFlowFields
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metric Flow")
	TMap<FString, FString> Data;

public:
	bool IsEmpty() const { return Data.Num() == 0; }
	int32 Num() const { return Data.Num(); }

	FMetricFlowFields& AddString(const FString& Key, const FString& Value)
	{
		if (!SetString(Key, Value))
		{
			UE_LOG(LogMetricFlow, Error, TEXT("MetricFlowPayload::Add - Failed to add key-value pair. Key is empty."));
		}
		return *this;
	}
	
	FMetricFlowFields& AddInt(const FString& Key, const int32 Value)
	{
		return AddString(Key, FString::FromInt(Value));
	}
	FMetricFlowFields& AddFloat(const FString& Key, const float Value)
	{
		return AddString(Key, FString::SanitizeFloat(Value));
	}
	FMetricFlowFields& AddBool(const FString& Key, const bool Value)
	{
		return AddString(Key, Value ? TEXT("true") : TEXT("false"));
	}
	FMetricFlowFields& AddMap(const TMap<FString, FString>& Values)
	{
		for (const TPair<FString, FString>& Pair : Values)
		{
			SetString(Pair.Key, Pair.Value);
		}
		return *this;
	}
	FMetricFlowFields& operator+(const FMetricFlowFields& Other)
	{
		for (const TPair<FString, FString>& Pair : Other.Data)
		{
			SetString(Pair.Key, Pair.Value);
		}
		return *this;
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
