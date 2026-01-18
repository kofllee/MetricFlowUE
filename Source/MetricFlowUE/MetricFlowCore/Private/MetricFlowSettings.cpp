#include "MetricFlowSettings.h"

void UMetricFlowSettings::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	bShutdownUsesFlush = ShutdownMode == EMetricFlowShutdownMode::Flush || ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist;
	
	if (MaxSendBatchSize < MinSendBatchSize)
	{
		MaxSendBatchSize = MinSendBatchSize;
	}
}
