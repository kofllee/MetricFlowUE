#include "MetricFlowSettings.h"

void UMetricFlowSettings::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	if (MaxSendBatchSize < MinSendBatchSize)
	{
		MaxSendBatchSize = MinSendBatchSize;
	}
}
