#include "MetricFlowCore.h"
#include "MetricFlowLog.h"

#define LOCTEXT_NAMESPACE "FMetricFlowCoreModule"

DEFINE_LOG_CATEGORY(LogMetricFlow);

void FMetricFlowCoreModule::StartupModule()
{
	UE_LOG(LogMetricFlow, Log, TEXT("MetricFlowCore module started"));
}

void FMetricFlowCoreModule::ShutdownModule()
{
	UE_LOG(LogMetricFlow, Log, TEXT("MetricFlowCore module shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMetricFlowCoreModule, MetricFlowUE)