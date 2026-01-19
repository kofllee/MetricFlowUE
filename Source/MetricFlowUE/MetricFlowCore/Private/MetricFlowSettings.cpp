#include "MetricFlowSettings.h"

void UMetricFlowSettings::PostInitProperties()
{
	Super::PostInitProperties();

	RecalculateShutdownFlag();
}

void UMetricFlowSettings::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	RecalculateShutdownFlag();
}

void UMetricFlowSettings::RecalculateShutdownFlag()
{
	bShutdownUsesFlush = ShutdownMode == EMetricFlowShutdownMode::Flush || ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist;
	bShutdownUsesPersist = ShutdownMode == EMetricFlowShutdownMode::Persist || ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist;
	
	if (MaxSendBatchSize < MinSendBatchSize)
	{
		MaxSendBatchSize = MinSendBatchSize;
	}
}
