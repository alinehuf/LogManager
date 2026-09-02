// =============================================================================
// LogManager.cpp
// LogManager Plugin for Unreal Engine 5
//
// Copyright (c) 2025 AIPIC Team, Research Center of Saint-Cyr Coetquidan
// All rights reserved.
//
// This file is part of the LogManager Plugin, licensed under the
// Creative Commons Attribution-NonCommercial 4.0 International License
// (CC BY-NC 4.0) with additional terms for software usage.
//
// You are free to:
//   - Share: copy and redistribute the material in any medium or format
//   - Adapt: remix, transform, and build upon the material
//
// Under the following terms:
//   - Attribution: You must give appropriate credit to the AIPIC Team,
//     Research Center of Saint-Cyr Coetquidan, provide a link to the license,
//     and indicate if changes were made.
//   - NonCommercial: You may not use the material for commercial purposes.
//
// Additional Terms for Software Usage:
//   - This plugin may be integrated into Unreal Engine projects for research,
//     educational, and non-commercial purposes only.
//   - Any derivative work or project using this plugin must retain this header
//     and provide attribution in accompanying documentation.
//   - Redistribution of this plugin, modified or unmodified, must preserve
//     this license header and must not be sold or sublicensed.
//
// Full license text: https://creativecommons.org/licenses/by-nc/4.0/legalcode
//
// For commercial licensing inquiries, contact:
//   AIPIC Team - Research Center of Saint-Cyr Coetquidan
// =============================================================================

#include "LogManagerModule.h"
#include "LibLogManager.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"

#define LOCTEXT_NAMESPACE "FLogManagerModule"

IMPLEMENT_MODULE(FLogManagerModule, LogManager)

// ----------------------------------------------------------------------------------------
// Getter for the singleton
// ----------------------------------------------------------------------------------------

FLogManagerModule& FLogManagerModule::Get()
{
	return FModuleManager::LoadModuleChecked<FLogManagerModule>("LogManager");
}

// ----------------------------------------------------------------------------------------
// Start / Stop the module (init/stop the thirdparty DLL and manage the handles)
// ----------------------------------------------------------------------------------------

void FLogManagerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Path to your log directory
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString baseDirectory = LogManagerConstants::GetBaseLogDirectory();
	// Directory Exists?
	if (!platformFile.DirectoryExists(*baseDirectory))
	{
		// Create the full directory tree if it doesn't exist yet
		platformFile.CreateDirectoryTree(*baseDirectory);
	}
}

void FLogManagerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.
	// For modules that support dynamic reloading,
	// we call this function before unloading the module.
	logManager->RequestShutdown();
	while(!logManager->IsShutdownFinished()) {
		FPlatformProcess::Sleep(0.1f);
	}
}

void FLogManagerModule::RequestShutdown() {
	logManager->RequestShutdown();
}
bool FLogManagerModule::IsShutdownFinished() const {
	return logManager->IsShutdownFinished();
}

// ----------------------------------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------------------------------

/**
 * Uses Unreal's reflection system (StaticEnum) to retrieve the string name
 * of an EEvent value. Returns "?" if the enum metadata is unavailable.
 */
FString eventToString(EEvent event) {
	const UEnum* Enum = StaticEnum<EEvent>();
	if (!Enum) return "?";
	return Enum->GetNameStringByValue((int64)event);
}

/**
 * Uses Unreal's reflection system (StaticEnum) to retrieve the string name
 * of an ESetting value. Returns "?" if the enum metadata is unavailable.
 */
FString settingToString(ESetting setting) {
	const UEnum* Enum = StaticEnum<ESetting>();
	if (!Enum) return "?";
	return Enum->GetNameStringByValue((int64)setting);
}

/**
 * Converts a C-string (ANSI) back to an ESetting enum value.
 * The string must match one of the enum entry names exactly (as returned by settingToString).
 * Falls back to the first enum value (index 0) on null input, unknown string, or missing enum.
 */
ESetting stringToSetting(const char* str)
{
	if (!str) return static_cast<ESetting>(0); // ou une valeur par défaut
	// Convertir le const char* en FString
	FString strF = UTF8_TO_TCHAR(str);
	// Récupérer la UEnum correspondant à ton enum
	const UEnum* EnumPtr = StaticEnum<ESetting>();
	if (!EnumPtr) return static_cast<ESetting>(0);
	// Convertir le FString en valeur enum
	int64 enumValue = EnumPtr->GetValueByName(FName(*strF));
	if (enumValue == INDEX_NONE)
		return static_cast<ESetting>(0);
	return static_cast<ESetting>(enumValue);
}

// ----------------------------------------------------------------------------------------
// Manage id for current user
// ----------------------------------------------------------------------------------------

long long FLogManagerModule::GetNextID(long long OriginalId)
{
	return logManager->GetNextID(OriginalId);
}

// ----------------------------------------------------------------------------------------
// Open/Close a log file
// ----------------------------------------------------------------------------------------

bool FLogManagerModule::OpenNewJSonFile(FString& filepath)
{
	if (!logManager->OpenNewJsonFile(filepath)) {
		UE_LOG(LogManagerMsg, Error, TEXT("FLogManagerModule: Fail to open %s for writing"), *filepath);
		return false;
	}
	return true;
}

bool FLogManagerModule::CloseJSonFile()
{
	if (!logManager->CloseJSonFile()) {
		UE_LOG(LogManagerMsg, Error, TEXT("FLogManagerModule: fail to close current file \n"));
		return false;
	}
	UE_LOG(LogManagerMsg, Log, TEXT("FLogManagerModule:	current file closed\n"));
	return true;
}

// ----------------------------------------------------------------------------------------
// Begin/End a log entry
// ----------------------------------------------------------------------------------------

bool FLogManagerModule::BeginLogEntry(EEvent ename, float gameTime, int nbFields)
{
	// Register a log entry for the current event
	// Capture the current wall-clock timestamp alongside the in-game time
	FDateTime date = FDateTime::Now();
	FString EventStr = eventToString(ename);
	FTCHARToUTF8 Converted(*EventStr); // persistant convertion
	const char* EventAnsi = Converted.Get();

	if (!logManager->NewJSonEvent(EventAnsi, gameTime, date.ToUnixTimestamp(), date.GetMillisecond(), (uint64)GFrameNumber, nbFields)) {
		UE_LOG(LogManagerMsg, Error, TEXT("FLogManagerModule: BeginLogEntry fail\n"));
		return false;
	}
	//UE_LOG(LogManagerMsg, Log, TEXT("Write new JSonEvent %s !\n"), *EventStr);
	return true;
}

bool FLogManagerModule::BeginConfigData(int nbFields) {
	if (!logManager->NewJsonConfigData(nbFields)) {
		UE_LOG(LogManagerMsg, Error, TEXT("FLogManagerModule: fail BeginConfigData with %d fields\n"), nbFields);
		return false;
	}
	//UE_LOG(LogManagerMsg, Log, TEXT("Start to write new ConfigData (%d)!\n"), nbFields);
	return true;
}


// ----------------------------------------------------------------------------------------
// Add data to a log entry
// ----------------------------------------------------------------------------------------

bool FLogManagerModule::AddStringData(const FString key, const FString& value)
{
	if (!logManager->AddStringData(TCHAR_TO_ANSI(*key), TCHAR_TO_ANSI(*value))) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddStringData: fail to write (%s:%s)\n"), UTF8_TO_TCHAR(*key), UTF8_TO_TCHAR(*value));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddIntData(const FString& key, int value)
{
	if (!logManager->AddIntData(TCHAR_TO_ANSI(*key), value)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddIntData: fail to write (%s:%d)\n"), UTF8_TO_TCHAR(*key), value);
		return false;
	}
	return true;
}

bool FLogManagerModule::AddFloatData(const FString& key, float value)
{
	if (!logManager->AddFloatData(TCHAR_TO_ANSI(*key), value)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddFloatData: fail to write (%s:%f)\n"), UTF8_TO_TCHAR(*key), value);
		return false;
	}
	return true;
}

bool FLogManagerModule::AddBoolData(const FString& key, bool value)
{
	if (!logManager->AddBoolData(TCHAR_TO_ANSI(*key), value)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddBoolData: fail to write (%s:%d)\n"), UTF8_TO_TCHAR(*key), value);
		return false;
	}
	return true;
}

bool FLogManagerModule::AddUIntData(const FString& key, unsigned int value)
{
	if (!logManager->AddUIntData(TCHAR_TO_ANSI(*key), value)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddUIntData: fail to write (%s:%u)\n"), UTF8_TO_TCHAR(*key), value);
		return false;
	}
	return true;
}

bool FLogManagerModule::AddLongLongData(const FString& key, signed long long value)
{
	if (!logManager->AddLongLongData(TCHAR_TO_ANSI(*key), value)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddLongLongData: fail to write (%s:%lld)\n"), UTF8_TO_TCHAR(*key), value);
		return false;
	}
	return true;
}

bool FLogManagerModule::AddULongLongData(const FString& key, unsigned long long value)
{
	if (!logManager->AddULongLongData(TCHAR_TO_ANSI(*key), value)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddULongLongData: fail to write (%s:%lld)\n"), UTF8_TO_TCHAR(*key), value);
		return false;
	}
	return true;
}

bool FLogManagerModule::AddComposedData(const FString& key, unsigned int nbSubPairs)
{
	if (!logManager->AddComposedData(TCHAR_TO_ANSI(*key), nbSubPairs)) {
		UE_LOG(LogManagerMsg, Error, TEXT("AddComposedData: fail to write (%s:%d)\n"), UTF8_TO_TCHAR(*key), nbSubPairs);
		return false;
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Get a json config file content
// ----------------------------------------------------------------------------------------

TSharedPtr<FJsonObject> FLogManagerModule::LoadConfigJson(const FString& filePath)
{
	const int32 maxRetries = 10;
	const float delay = 0.1f; // 100 ms
	int32 attempt = 0;

	// file exists? 
	if (!FPaths::FileExists(filePath))
	{
		UE_LOG(LogManagerMsg, Error, TEXT("LoadConfigJson: the file (%s) does not exist\n"), *filePath);
		return nullptr;
	}
	// Retry loop: the file may be briefly locked if it was just written by CreateNewConfigFile()
	FString fileContent = "";
	while (attempt < maxRetries)
	{
		if (FFileHelper::LoadFileToString(fileContent, *filePath))
			break;

		// wait before retry
		FPlatformProcess::Sleep(delay);
		attempt++;
	}
	// nothing read?
	if (attempt == maxRetries) {
		UE_LOG(LogManagerMsg, Error, TEXT("LoadConfigJson: fail to read file content after %d tries (%s)\n"), maxRetries, *filePath);
		return nullptr;
	}
	else if (fileContent == "") {
		UE_LOG(LogManagerMsg, Error, TEXT("LoadConfigJson: fail to get file content or file is empty (%s)\n"), *filePath);
		return nullptr;
	}
	// Deserialize the JSON content
	// The config file is expected to be a JSON array containing a single object: [{ ... }]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(fileContent);
	if (FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		TSharedPtr<FJsonObject> Object = JsonArray[0]->AsObject();
		return Object;
	}
	UE_LOG(LogManagerMsg, Error, TEXT("Failed to deserialize JSON data: %s"), *filePath);
	return nullptr;
}


#undef LOCTEXT_NAMESPACE
