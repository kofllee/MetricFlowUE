#include "MetricFlowJson.h"

#include "MetricFlowEvent.h"
#include "MetricFlowFields.h"


TSharedPtr<FJsonObject> MetricFlowJson::SerializeFieldsToJson(const FMetricFlowFields& Fields)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Fields.Data)
	{
		Json->SetStringField(Pair.Key, Pair.Value);
	}
	return Json;
}

FString MetricFlowJson::SerializeAppendEventsToString(const FString& ProjectId, const FString& SessionId,
	const TArray<FMetricFlowEvent>& Events)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("projectId"),	ProjectId);
	Root->SetStringField(TEXT("op"),	TEXT("appendEvents"));
	Root->SetStringField(TEXT("sessionId"), SessionId);

	TArray<TSharedPtr<FJsonValue>> JsonEvents;
	JsonEvents.Reserve(Events.Num());
	
	for (const FMetricFlowEvent& Event : Events)
	{
		TSharedPtr<FJsonObject> JsonEvent = MakeShared<FJsonObject>();
		JsonEvent->SetStringField(TEXT("eventName"), Event.EventName.ToString());
		JsonEvent->SetStringField(TEXT("timestampUTC"), Event.TimestampUTC.ToIso8601());
		JsonEvent->SetStringField(TEXT("seq"), LexToString(Event.SequenceNumber));
		JsonEvent->SetObjectField(TEXT("eventContext"), SerializeFieldsToJson(Event.Context));
		TArray<TSharedPtr<FJsonValue>> Sheets;
		for (const FString& Sheet : Event.ExtraSheets)
		{
			if (!Sheet.IsEmpty())
				Sheets.Add(MakeShared<FJsonValueString>(Sheet));
		}

		if (Sheets.Num() > 0)
		{
			JsonEvent->SetArrayField(TEXT("extraSheets"), Sheets);
		}
		JsonEvent->SetObjectField(TEXT("payload"), SerializeFieldsToJson(Event.Payload));

		JsonEvents.Add(MakeShared<FJsonValueObject>(JsonEvent));
	}

	Root->SetArrayField(TEXT("events"), JsonEvents);

	FString out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&out);
	if (FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		Writer->Close();
		return out;
	}
	
	UE_LOG(LogMetricFlow, Error, TEXT("MetricFlowJson::SerializeAppendEventsToString: Failed to serialize JSON"));
	return FString();
}

FString MetricFlowJson::SerializeUpsertSessionToString(const FString& ProjectId, const FString& SessionId,
	const FMetricFlowFields& Session)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("projectId"),	ProjectId);
	Root->SetStringField(TEXT("op"),	TEXT("upsertSession"));
	Root->SetStringField(TEXT("sessionId"), SessionId);
	Root->SetObjectField(TEXT("session"), SerializeFieldsToJson(Session));
	
	FString out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&out);
	if (FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		Writer->Close();
		return out;
	}
	
	UE_LOG(LogMetricFlow, Error, TEXT("MetricFlowJson::SerializeUpsertSessionToString: Failed to serialize JSON"));
	return FString();
}
