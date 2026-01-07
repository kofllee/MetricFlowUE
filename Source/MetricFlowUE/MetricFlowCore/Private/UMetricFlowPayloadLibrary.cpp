#include "UMetricFlowPayloadLibrary.h"

FMetricFlowPayload UMetricFlowPayloadLibrary::CreateMetricFlowPayload()
{
	return FMetricFlowPayload();
}

FMetricFlowPayload UMetricFlowPayloadLibrary::AddString(FMetricFlowPayload Payload, const FString& Key,
	const FString& Value)
{
	Payload.AddString(Key, Value);
	return Payload;
}

FMetricFlowPayload UMetricFlowPayloadLibrary::AddInt(FMetricFlowPayload Payload, const FString& Key, const int32 Value)
{
	Payload.AddInt(Key, Value);
	return Payload;
}

FMetricFlowPayload UMetricFlowPayloadLibrary::AddFloat(FMetricFlowPayload Payload, const FString& Key,
	const float Value)
{
	Payload.AddFloat(Key, Value);
	return Payload;
}

FMetricFlowPayload UMetricFlowPayloadLibrary::AddBool(FMetricFlowPayload Payload, const FString& Key, const bool Value)
{
	Payload.AddBool(Key, Value);
	return Payload;
}
