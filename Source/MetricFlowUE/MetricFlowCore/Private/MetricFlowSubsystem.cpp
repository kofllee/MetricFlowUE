#include "MetricFlowSubsystem.h"

#include "FMetricFlowQueueStore.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "MetricFlowDefaultEventContextProvider.h"
#include "MetricFlowDefaultSessionContextProvider.h"
#include "MetricFlowEvent.h"
#include "MetricFlowFields.h"
#include "MetricFlowJson.h"
#include "Containers/Ticker.h"

namespace MetricFLow
{
	static bool ShouldRetry(bool bWasSuccessful, int32 ResponseCode)
	{
		if (!bWasSuccessful) return true;
		if (ResponseCode == 0 || ResponseCode == -1) return true;
		if (ResponseCode >= 500 && ResponseCode < 600) return true;
		return false;
	}

}

void UMetricFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMetricFlowSettings* Settings = UMetricFlowSettings::Get();
	if (!Settings)
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: Settings not found"));
		bEnabledRuntime = false;
		return;
	}

#if UE_BUILD_SHIPPING
	if (!Settings->bSendInShipping)
	{
		bEnabledRuntime = false;
		return;
	}
#endif

	bEnabledRuntime = Settings->bEnableMetricFlow;

	ProjectId = Settings->ProjectId;
	ProxyApiKey = Settings->ProxyApiKey;
	
	ActiveEndpointURL = Settings->GetActiveEndpointURL();

	MaxQueueSize = Settings->MaxQueueSize;
	MaxShutdownBatches = Settings->MaxShutdownBatches;
	MinBatchSize = FMath::Max(1, Settings->MinSendBatchSize);
	MaxBatchSize = FMath::Max(MinBatchSize, Settings->MaxSendBatchSize);
	EventSeq = 0;
	MaxLastBatchSize = FMath::Max(1, Settings->MaxLastBatchSize);
	DefaultFlushInterval = Settings->FlushIntervalSeconds;
	CurrentFlushInterval = Settings->FlushIntervalSeconds;
	FlushRetryIntervalMultiplier = Settings->FlushRetryBackoffMultiplier;
	RetryMaxAttempts = Settings->RetryMaxAttempts;
	RetryCount = 0;

	ShutdownMode = Settings->ShutdownMode;
	ShutdownFlushTimeMs = Settings->ShutdownFlushTimeMs;
	bShutdownWaitForResponses = Settings->bShutdownWaitForResponses;
	
	TimeoutSeconds = Settings->RequestTimeoutSeconds;

	if (ActiveEndpointURL.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: No active endpoint"));
		bEnabledRuntime = false;
		return;
	}
	if (MaxQueueSize < 0)
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: Invalid MaxQueueSize=%d"), MaxQueueSize);
		bEnabledRuntime = false;
		return;
	}
	if (ProjectId.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::Initialize: No ProjectId"));
		bEnabledRuntime = false;
		return;
	}

	if (!bEnabledRuntime)
	{
		return;
	}

	RequestsInFlight = 0;

	SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	SessionStartedAtUTC = FDateTime::UtcNow().ToIso8601();
	SessionEndedAtUTC = TEXT("");
	LoadProvidersFromSettings(Settings);
	RebuildSessionContextCache();

	UE_LOG(
		LogMetricFlow,
		Log,
		TEXT("MetricFlow init: enabled=%d session=%s"),
		bEnabledRuntime ? 1 : 0,
		*SessionId
	);

	TrySendUpsertSession();

	OutboxTimerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime)
		{
			SendNextOutbox();
			return true;
		}),
		DefaultFlushInterval
	);

	FlushTimerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime)
		{
			TrySendNext();
			return true;
		}),
		CurrentFlushInterval
	);
}

void UMetricFlowSubsystem::Deinitialize()
{
	if (bEnabledRuntime)
	{
		TurnOffTimers();
		
		SessionEndedAtUTC = FDateTime::UtcNow().ToIso8601();
		double EndTime = FPlatformTime::Seconds() + (ShutdownFlushTimeMs / 1000.0);
		TrySendUpsertSession();

		if (ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist || ShutdownMode == EMetricFlowShutdownMode::Flush)
			TrySendLastEvents(EndTime);

		if (bShutdownWaitForResponses)
		{
			const float SleepInterval = 0.01f;
			while (RequestsInFlight > 0 && FPlatformTime::Seconds() < EndTime)
			{
				FHttpModule::Get().GetHttpManager().Tick(SleepInterval);

				if(FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().PumpMessages();
					FSlateApplication::Get().Tick();
				}
				
				FPlatformProcess::Sleep(SleepInterval);
			}
		}
		
		if (ShutdownMode == EMetricFlowShutdownMode::FlushThenPersist || ShutdownMode == EMetricFlowShutdownMode::Persist)
		{
			PersistQueueToDisk();
		}
	}

	Super::Deinitialize();
}


void UMetricFlowSubsystem::RecordEvent(const FName& EventName, const FMetricFlowFields& ExtraContext, const FMetricFlowFields& Payload, const TArray<FString>& ExtraSheets)
{
	FMetricFlowFields EventContext;
	for (const UMetricFlowContextProviderBase* Provider : EventContextProviders)
	{
		TMap<FString, FString> ProviderFields;
		Provider->CollectFields(this, ProviderFields);
		EventContext = EventContext.AddMap(ProviderFields);
	}
	
	const FMetricFlowEvent Event = CreateMetricFlowEvent(EventName, EventSeq, EventContext + ExtraContext, Payload, ExtraSheets);
	EventSeq++;
	
	EventQueue.Add(Event);
	const int32 Overflow = EventQueue.Num() - MaxQueueSize;
	if (Overflow > 0)
		EventQueue.RemoveAt(0, Overflow, EAllowShrinking::No);
}

void UMetricFlowSubsystem::RecordEventMap(const FName& EventName, const TMap<FString, FString>& ExtraContextMap, const TMap<FString, FString>& PayloadMap, const TArray<FString>& ExtraSheets)
{
	
	const FMetricFlowFields ExtraContext = FMetricFlowFields().AddMap(ExtraContextMap);
	const FMetricFlowFields Payload = FMetricFlowFields().AddMap(PayloadMap);

	RecordEvent(EventName, ExtraContext, Payload, ExtraSheets);
}


FMetricFlowEvent UMetricFlowSubsystem::CreateMetricFlowEvent(const FName& EventName, const int64 Seq, const FMetricFlowFields& Context,
	const FMetricFlowFields& Payload, const TArray<FString>& ExtraSheets)
{
	FMetricFlowEvent Event = FMetricFlowEvent();
	
	Event.EventName = EventName;
	Event.Context = Context;
	Event.TimestampUTC = FDateTime::UtcNow();
	Event.SequenceNumber = Seq;
	Event.Payload = Payload;
	Event.ExtraSheets = ExtraSheets;
	return Event;
}

bool UMetricFlowSubsystem::BuildUpsertSessionRequest(FMetricFlowPendingRequest& OutReq)
{
	FMetricFlowFields Session = FMetricFlowFields();
	Session.AddString(TEXT("startedAtUTC"), SessionStartedAtUTC);
	Session.AddString(TEXT("endedAtUTC"), SessionEndedAtUTC);
	Session.AddMap(CachedSessionContext);

	const FString Json = MetricFlowJson::ToString(MetricFlowJson::SerializeUpsertSessionToJson(ProjectId, SessionId, Session));
	if (Json.IsEmpty()) return false;

	OutReq = FMetricFlowPendingRequest();
	OutReq.Op = EMetricFlowOp::UpsertSession;
	OutReq.Json = Json;
	return true;
}

bool UMetricFlowSubsystem::BuildAppendEventsRequest(FMetricFlowPendingRequest& OutReq, const int32 MinSendBatchSize, const int32 MaxSendBatchSize)
{
	if (EventQueue.Num() == 0) return false;
	
	const int32 Count = FMath::Min(MaxSendBatchSize, EventQueue.Num());
	if (Count < MinSendBatchSize) return false;

	TArray<FMetricFlowEvent> Batch;
	Batch.Reserve(Count);
	Batch.Append(EventQueue.GetData(), Count);

	const FString Json = MetricFlowJson::ToString(MetricFlowJson::SerializeAppendEventsToJson(ProjectId, SessionId, Batch));
	if (Json.IsEmpty())
	{
		return false;
	}

	OutReq = FMetricFlowPendingRequest();
	OutReq.Op = EMetricFlowOp::AppendEvents;
	OutReq.Json = Json;
	OutReq.SendBatch = MoveTemp(Batch);
	
	return true;
}

void UMetricFlowSubsystem::TrySendNext()
{
	if (bRetrySessionUpsert)
	{
		TrySendUpsertSession();
		return;
	}

	TrySendAppendEvents();
}

void UMetricFlowSubsystem::TrySendUpsertSession()
{
	FMetricFlowPendingRequest Req;
	if (!BuildUpsertSessionRequest(Req)) return;
	Req.InFlightPolicy = EMetricFlowInFlightPolicy::Ignore;
	SendRequest(MoveTemp(Req));
}

void UMetricFlowSubsystem::TrySendAppendEvents()
{
	FMetricFlowPendingRequest Req;
	if (!BuildAppendEventsRequest(Req, MinBatchSize, MaxBatchSize)) return;
	const int32 SentCount = Req.SendBatch.Num();
	
	if (!SendRequest(MoveTemp(Req))) return;

	if (SentCount > 0)
	{
		EventQueue.RemoveAt(0, SentCount, EAllowShrinking::No);
	}
}

bool UMetricFlowSubsystem::SendRequest(FMetricFlowPendingRequest Req)
{
	if (!bEnabledRuntime) return false;
	if (ActiveEndpointURL.IsEmpty()) return false;
	if (Req.InFlightPolicy == EMetricFlowInFlightPolicy::WaitUntilIdle && RequestsInFlight != 0)
	{
		return false;
	}
	if (Req.Json.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Warning, TEXT("UMetricFlowSubsystem::SendRequest: Empty JSON for %s"), ToString(Req.Op));
		return false;
	}

	RequestsInFlight++;

	TWeakObjectPtr<UMetricFlowSubsystem> WeakThis(this);
	Sender.PostJson(ActiveEndpointURL, ProxyApiKey, Req.Json, TimeoutSeconds,
		[WeakThis, Req](bool bWasSuccessful, int32 ResponseCode, const FString& ResponseBody) 
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->OnRequestCompleted(Req, bWasSuccessful, ResponseCode, ResponseBody);
		});
	
	return true;
}

void UMetricFlowSubsystem::OnRequestCompleted(const FMetricFlowPendingRequest& Req, const bool bWasSuccessful, const int32 ResponseCode,
	const FString& ResponseBody)
{
	const bool bOk = bWasSuccessful && ResponseCode >= 200 && ResponseCode < 300;
	const bool bRetry = MetricFLow::ShouldRetry(bWasSuccessful, ResponseCode);

	if (bOutboxDraining)
	{
		if (bOk)
		{
			FMetricFlowQueueStore::DeleteFile(OutboxCurrentFile);
		}
		bOutboxDraining = false;
		OutboxCurrentFile.Reset();
	}
	
	if (bOk)
	{
		UE_LOG(LogMetricFlow, Log, TEXT("MetricFlow %s succeeded: code=%d body=%s"),ToString(Req.Op), ResponseCode, *ResponseBody);
		RequestsInFlight--;
		RetryCount = 0;
		CurrentFlushInterval = DefaultFlushInterval;
		
		if (FlushTimerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(FlushTimerHandle);
			FlushTimerHandle.Reset();
		}
		FlushTimerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([this](float DeltaTime)
			{
				TrySendNext();
				return true;
			}),
			CurrentFlushInterval
		);
		return;
	}

	UE_LOG(LogMetricFlow, Warning, TEXT("MetricFlow %s failed: code=%d body=%s"), ToString(Req.Op), ResponseCode, *ResponseBody);
	if (Req.Op == EMetricFlowOp::AppendEvents && Req.SendBatch.Num() > 0)
	{
		EventQueue.Insert(Req.SendBatch, 0);
	}

	if (bRetry && RetryCount < RetryMaxAttempts)
	{
		RetryCount++;
		CurrentFlushInterval *= FlushRetryIntervalMultiplier;

		if (FlushTimerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(FlushTimerHandle);
			FlushTimerHandle.Reset();
		}
		FlushTimerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([this](float DeltaTime)
			{
				TrySendNext();
				return true;
			}),
			CurrentFlushInterval
		);
		
		UE_LOG(LogMetricFlow, Log, TEXT("UMetricFlowSubsystem::OnRequestCompleted: Scheduling retry %d/%d in %.2f seconds"),
			RetryCount, RetryMaxAttempts, CurrentFlushInterval);
	}
	else
	{
		TurnOffTimers();
		bEnabledRuntime = false;
		UE_LOG(LogMetricFlow, Error, TEXT("UMetricFlowSubsystem::OnRequestCompleted: Max retries reached or non-retriable error, turning off MetricFlow runtime"));
	}
	
	RequestsInFlight--;
}

void UMetricFlowSubsystem::TrySendLastEvents(const double EndTime)
{
	int32 BatchesSent = 0;
	while (EventQueue.Num() > 0 && BatchesSent < MaxShutdownBatches)
	{
		if (FPlatformTime::Seconds() > EndTime) return;
		
		FMetricFlowPendingRequest Req;
		if (!BuildAppendEventsRequest(Req, 1, MaxLastBatchSize)) return;
		Req.InFlightPolicy = EMetricFlowInFlightPolicy::Ignore;
		const int32 SentCount = Req.SendBatch.Num();

		if (!SendRequest(MoveTemp(Req))) continue;
		BatchesSent++;
		if (SentCount > 0)
		{
			EventQueue.RemoveAt(0, SentCount, EAllowShrinking::No);
		}
	}
}

void UMetricFlowSubsystem::PersistQueueToDisk()
{
	if (EventQueue.Num() <= 0) return;

	const bool bSaved = FMetricFlowQueueStore::SaveBatches(ProjectId, SessionId, EventQueue, MaxLastBatchSize);

	UE_LOG(LogMetricFlow, Log, TEXT("UMetricFlowSubsystem::PersistQueueToDisk: Saved %d events to disk: %hs"), EventQueue.Num(), bSaved ? "success" : "failure");
}

void UMetricFlowSubsystem::SendNextOutbox()
{
	if (bOutboxDraining) return;
	
	TArray<FString> Files = FMetricFlowQueueStore::ListQueueFiles();
	if (Files.Num() == 0)
	{
		if (OutboxTimerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(OutboxTimerHandle);
			OutboxTimerHandle.Reset();
		}
		return;
	}

	const FString& FilePath = Files[0];

	FString RequestJson;
	if (!FMetricFlowQueueStore::LoadBatch(FilePath,  RequestJson) || RequestJson.IsEmpty())
	{
		UE_LOG(LogMetricFlow, Warning, TEXT("UMetricFlowSubsystem::SendNextOutbox: Failed to load batch from %s, deleting"), *FilePath);
		FMetricFlowQueueStore::DeleteFile(FilePath);
		return;
	}

	FMetricFlowPendingRequest Req;
	Req.Op = EMetricFlowOp::AppendEvents;
	Req.InFlightPolicy = EMetricFlowInFlightPolicy::WaitUntilIdle;
	Req.Json = RequestJson;
	
	bOutboxDraining = true;
	OutboxCurrentFile = FilePath;

	if (!SendRequest(MoveTemp(Req)))
	{
		bOutboxDraining = false;
		OutboxCurrentFile.Reset();
	}
}

void UMetricFlowSubsystem::LoadProvidersFromSettings(const UMetricFlowSettings* Settings)
{
	for (TSubclassOf<UMetricFlowContextProviderBase> ProviderClass : Settings->SessionContextProviders)
	{
		if (!ProviderClass) continue;
		if (UMetricFlowContextProviderBase* Provider = NewObject<UMetricFlowContextProviderBase>(this, ProviderClass))
		{
			SessionContextProviders.Add(Provider);
		}
	}
	if (SessionContextProviders.Num() == 0)
	{
		if (UMetricFlowContextProviderBase* Provider =
			NewObject<UMetricFlowContextProviderBase>(this, UMetricFlowDefaultSessionContextProvider::StaticClass()))
		{
			SessionContextProviders.Add(Provider);
		}
	}

	for (TSubclassOf<UMetricFlowContextProviderBase> ProviderClass : Settings->EventContextProviders)
	{
		if (!ProviderClass) continue;
		if (UMetricFlowContextProviderBase* Provider = NewObject<UMetricFlowContextProviderBase>(this, ProviderClass))
		{
			EventContextProviders.Add(Provider);
		}
	}
	if (EventContextProviders.Num() == 0)
	{
		if (UMetricFlowContextProviderBase* Provider =
			NewObject<UMetricFlowContextProviderBase>(this, UMetricFlowDefaultEventContextProvider::StaticClass()))
		{
			EventContextProviders.Add(Provider);
		}
	}
}

void UMetricFlowSubsystem::RebuildSessionContextCache()
{
	CachedSessionContext.Reset();

	for (const UMetricFlowContextProviderBase* Provider : SessionContextProviders)
	{
		if (!Provider) continue;

		TMap<FString, FString> ProviderFields;
		Provider->CollectFields(this, ProviderFields);

		for (const TPair<FString, FString>& Pair : ProviderFields)
		{
			CachedSessionContext.Add(Pair.Key, Pair.Value);
		}
	}
}

void UMetricFlowSubsystem::TurnOffTimers()
{
	if (OutboxTimerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(OutboxTimerHandle);
		OutboxTimerHandle.Reset();
	}

	if (FlushTimerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FlushTimerHandle);
		FlushTimerHandle.Reset();
	}
}
