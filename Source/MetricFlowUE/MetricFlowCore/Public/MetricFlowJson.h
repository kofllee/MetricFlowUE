#pragma once

struct FMetricFlowFields;
struct FMetricFlowEvent;

namespace MetricFlowJson
{
	TSharedPtr<FJsonObject> SerializeFieldsToJson(const FMetricFlowFields& Fields);

	TSharedPtr<FJsonObject> SerializeAppendEventsToJson(
		const FString& ProjectId,
		const FString& SessionId,
		const TArray<FMetricFlowEvent>& Events);

	TSharedPtr<FJsonObject> SerializeUpsertSessionToJson(
		const FString& ProjectId,
		const FString& SessionId,
		const FMetricFlowFields& SessionSnapshot);
	
	FString ToString(const TSharedPtr<FJsonObject>& Json);
}
