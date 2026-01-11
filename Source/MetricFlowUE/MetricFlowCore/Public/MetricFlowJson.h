#pragma once

struct FMetricFlowContext;
struct FMetricFlowEvent;

namespace MetricFlowJson
{
	FString SerializeBatchToString(
		const FString& ProjectId,
		const FString& DefaultSheet,
		const TArray<FMetricFlowEvent>& Events);
}
