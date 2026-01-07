#pragma once

struct FMetricFlowContext;
struct FMetricFlowEvent;

namespace MetricFlowJson
{
	FString SerializeBatchToString(
		const FString& ProjectToken,
		const FString& DefaultSheet,
		const TArray<FMetricFlowEvent>& Events);
}
