#pragma once

#include "CoreMinimal.h"
#include "MetricFlowContextProviderBase.h"
#include "MetricFlowDefaultEventContextProvider.h"
#include "MetricFlowDefaultSessionContextProvider.h"
#include "Engine/DeveloperSettings.h"
#include "MetricFlowSettings.generated.h"


UENUM(BlueprintType)
enum class EMetricFlowShutdownMode : uint8
{
	None,
	Flush,
	Persist,
	FlushThenPersist,
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Metric Flow UE"))
class UMetricFlowSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	virtual FName GetCategoryName() const override
	{
		return TEXT("Plugins");
	}

	UPROPERTY(EditAnywhere, Config, Category="General")
	bool bEnableMetricFlow = true;

	UPROPERTY(EditAnywhere, Config, Category="General")
	bool bSendInShipping = true;


	UPROPERTY(EditAnywhere, Config, Category="General")
	TArray<TSubclassOf<UMetricFlowSessionContextProviderBase>> SessionContextProviders = {
		UMetricFlowDefaultSessionContextProvider::StaticClass()
	};

	UPROPERTY(EditAnywhere, Config, Category="General")
	TArray<TSubclassOf<UMetricFlowEventContextProviderBase>> EventContextProviders = {
		UMetricFlowDefaultEventContextProvider::StaticClass()
	};
	

	UPROPERTY(EditAnywhere, Config, Category="Destination", meta=(DisplayName="Project Id"))
	FString ProjectId;

	UPROPERTY(EditAnywhere, Config, Category="Destination", meta=(DisplayName="Proxy API Key"))
	FString ProxyApiKey;

	UPROPERTY(EditAnywhere, Config, Category="Destination|Dev", meta=(DisplayName="Dev Endpoint URL"))
	FString DevEndpointURL;

	UPROPERTY(EditAnywhere, Config, Category="Destination|Shipping", meta=(DisplayName="Shipping Endpoint URL"))
	FString ShippingEndpointURL;


	UPROPERTY(EditAnywhere, Config, Category="Batching", meta=(ClampMin="1", UIMin="1"))
	int32 MinSendBatchSize = 15;
	
	UPROPERTY(EditAnywhere, Config, Category="Batching", meta=(ClampMin="1", UIMin="1"))
	int32 MaxSendBatchSize = 30;

	UPROPERTY(EditAnywhere, Config, Category="Batching", meta=(ClampMin="0.1", UIMin="0.1"))
	float FlushIntervalSeconds = 5.0f;

	UPROPERTY(EditAnywhere, Config, Category="Batching", meta=(ClampMin="10", UIMin="10"))
	int32 MaxQueueSize = 2000;
	

	UPROPERTY(EditAnywhere, Config, Category="Reliability", meta=(ClampMin="0.1", UIMin="0.1"))
	float RequestTimeoutSeconds = 10.0f;

	UPROPERTY(EditAnywhere, Config, Category="Shutdown")
	EMetricFlowShutdownMode ShutdownMode = EMetricFlowShutdownMode::FlushThenPersist;

	UPROPERTY(EditAnywhere, Config, Category="Shutdown", meta=(ClampMin="100", UIMin="100", EditCondition="bShutdownUsesFlush", EditConditionHides))
	float ShutdownFlushTimeMs = 3000.0f;

	UPROPERTY(EditAnywhere, Config, Category="Shutdown", meta=(ClampMin="1", UIMin="1", EditCondition="bShutdownUsesFlush||bShutdownUsesPersist", EditConditionHides))
	int32 MaxLastBatchSize = 200;

	UPROPERTY(EditAnywhere, Config, Category="Shutdown", meta=(ClampMin="1", UIMin="1", EditCondition="bShutdownUsesFlush", EditConditionHides))
	int32 MaxShutdownBatches = 3;

	UPROPERTY(EditAnywhere, Config, Category="Shutdown")
	bool bShutdownWaitForResponses = false;

	

	FString GetActiveEndpointURL() const
	{
#if UE_BUILD_SHIPPING
		return ShippingEndpointURL;
#endif
		return DevEndpointURL;
	};

	static const UMetricFlowSettings* Get()
	{
		return GetDefault<UMetricFlowSettings>();
	};
	
private:
	UPROPERTY(Transient)
	bool bShutdownUsesFlush = ShutdownMode == EMetricFlowShutdownMode::Flush || ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist;
	UPROPERTY(Transient)
	bool bShutdownUsesPersist = ShutdownMode == EMetricFlowShutdownMode::Persist || ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
	
};

