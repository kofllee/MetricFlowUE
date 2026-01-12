#include "MetricFlowCore.h"
#include "MetricFlowLog.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FMetricFlowCoreModule"

DEFINE_LOG_CATEGORY(LogMetricFlow);

void FMetricFlowCoreModule::StartupModule()
{
	FString VersionName = IPluginManager::Get().FindPlugin(TEXT("MetricFlowUE"))->GetDescriptor().VersionName;
	
	UE_LOG(LogMetricFlow, Log, TEXT("MetricFlow (%s) module started"), *VersionName);
}

void FMetricFlowCoreModule::ShutdownModule()
{
	UE_LOG(LogMetricFlow, Log, TEXT("MetricFlow module shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMetricFlowCoreModule, MetricFlowCore)