#include "UMetricFlowFieldsLibrary.h"

#include "Internationalization/TextPackageNamespaceUtil.h"

FMetricFlowFields UMetricFlowFieldsLibrary::CreateMetricFlowFields()
{
	return FMetricFlowFields();
}

FMetricFlowFields UMetricFlowFieldsLibrary::AddString(FMetricFlowFields Fields, const FString& Key,
	const FString& Value)
{
	Fields.AddString(Key, Value);
	return Fields;
}

FMetricFlowFields UMetricFlowFieldsLibrary::AddInt(FMetricFlowFields Fields, const FString& Key, const int32 Value)
{
	Fields.AddInt(Key, Value);
	return Fields;
}

FMetricFlowFields UMetricFlowFieldsLibrary::AddFloat(FMetricFlowFields Fields, const FString& Key,
	const float Value)
{
	Fields.AddFloat(Key, Value);
	return Fields;
}

FMetricFlowFields UMetricFlowFieldsLibrary::AddBool(FMetricFlowFields Fields, const FString& Key, const bool Value)
{
	Fields.AddBool(Key, Value);
	return Fields;
}

FMetricFlowFields UMetricFlowFieldsLibrary::AddMap(FMetricFlowFields Fields, const TMap<FString, FString>& Values)
{
	Fields.AddMap(Values);
	return Fields;
}

FMetricFlowFields UMetricFlowFieldsLibrary::CombineFields(FMetricFlowFields A, const FMetricFlowFields& B)
{
	return A + B;
}
