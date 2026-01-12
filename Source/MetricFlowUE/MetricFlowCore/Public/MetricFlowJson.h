#pragma once

struct FMetricFlowFields;
struct FMetricFlowEvent;

namespace MetricFlowJson
{
	TSharedPtr<FJsonObject> SerializeFieldsToJson(const FMetricFlowFields& Fields);

	FString SerializeAppendEventsToString(
		const FString& ProjectId,
		const FString& SessionId,
		const TArray<FMetricFlowEvent>& Events);

	FString SerializeUpsertSessionToString(
		const FString& ProjectId,
		const FMetricFlowFields& SessionSnapshot);
}
