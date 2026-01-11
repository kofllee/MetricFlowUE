#pragma once

struct FMetricFlowFields;
struct FMetricFlowEvent;

namespace MetricFlowJson
{
	FString SerializeBatchToString(
		const FString& ProjectId,
		const FMetricFlowFields& SessionContext,
		const TArray<FMetricFlowEvent>& Events);
}
