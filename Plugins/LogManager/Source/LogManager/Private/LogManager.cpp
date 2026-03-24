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

#include "LogManager.h"
#include "LibLogManager.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"

#define LOCTEXT_NAMESPACE "FLogManagerModule"

IMPLEMENT_MODULE(FLogManagerModule, LogManager)

DEFINE_LOG_CATEGORY(LogManager);

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
	FString reasonFails;
	if (!Init(reasonFails))
		UE_LOG(LogManager, Error, TEXT("Error during initialization of the logger: % s\n"), *reasonFails);
}

void FLogManagerModule::ShutdownModule()
{
	Shutdown();
}

// ----------------------------------------------------------------------------------------
// Initialize/stop the DLL : get library handle and each library entry point
// ----------------------------------------------------------------------------------------

void* FLogManagerModule::GetLogManagerLibraryHandle(FString& reasonFails)
{
	void* libraryHandle = nullptr;
#if PLATFORM_WINDOWS	
	// Get the base directory of this plugin
	FString lBaseDir = IPluginManager::Get().FindPlugin("LogManager")->GetBaseDir();	
	// Add on the relative location of the third party dll and load it
	FString libraryPath = FPaths::Combine(*lBaseDir, TEXT("Source/ThirdParty/LibLogManager/Windows/liblogmanager.dll"));
	// check the file path
	if (!FPaths::FileExists(libraryPath))
	{
		reasonFails = "File '" + libraryPath + "' doesn't exist !";
		return nullptr;
	}
	// Get the handle
	libraryHandle = FPlatformProcess::GetDllHandle(*libraryPath);
#elif PLATFORM_ANDROID
	libraryHandle = FPlatformProcess::GetDllHandle(TEXT("liblogmanager.so"));
#endif
	// Check the handle
	if (libraryHandle == NULL)
		reasonFails = "Loading LogManager library failed.";
	return libraryHandle;
}

void* FLogManagerModule::GetLogManagerLibraryEntryPoint(const char* entryPointName, FString& reasonFails)
{
	if (!logManagerLibraryHandle)
	{
		reasonFails = TEXT("Library handle is null");
		return nullptr;
	}
	void* entyPoint = FPlatformProcess::GetDllExport(
		logManagerLibraryHandle,
		UTF8_TO_TCHAR(entryPointName)
	);
	if (!entyPoint)
	{
		reasonFails = FString::Printf(
			TEXT("Loading '%s' failed!"),
			UTF8_TO_TCHAR(entryPointName)
		);
	}
	return entyPoint;
}

bool FLogManagerModule::Init(FString& reasonFails)
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

	// get the library handle
	logManagerLibraryHandle = GetLogManagerLibraryHandle(reasonFails);
	if (logManagerLibraryHandle == nullptr) return false;

	// get all the entry points
	// Each function pointer is resolved by name from the shared library.
	// If any entry point fails to resolve, initialization is aborted.
	l_initLogger = (initLoggerSignature)GetLogManagerLibraryEntryPoint("initLogger", reasonFails);
	if (l_initLogger == nullptr) return false;

	l_shutdownLogger = (shutdownLoggerSignature)GetLogManagerLibraryEntryPoint("shutdownLogger", reasonFails);
	if (l_shutdownLogger == nullptr) return false;

	l_openNewJSonFile = (openNewJSonFileSignature)GetLogManagerLibraryEntryPoint("openNewJSonFile", reasonFails);
	if (l_openNewJSonFile == nullptr) return false;

	l_flushAndCloseJSonFile = (flushAndCloseJSonFileSignature)GetLogManagerLibraryEntryPoint("flushAndCloseJSonFile", reasonFails);
	if (l_flushAndCloseJSonFile == nullptr) return false;

	l_lockWrite = (lockWriteSignature)GetLogManagerLibraryEntryPoint("lockWrite", reasonFails);
	if (l_lockWrite == nullptr) return false;

	l_unlockWrite = (unlockWriteSignature)GetLogManagerLibraryEntryPoint("unlockWrite", reasonFails);
	if (l_unlockWrite == nullptr) return false;

	l_newJSonEvent = (newJSonEventSignature)GetLogManagerLibraryEntryPoint("newJSonEvent", reasonFails);
	if (l_newJSonEvent == nullptr) return false;

	l_newJSonConfigData = (newJSonConfigData)GetLogManagerLibraryEntryPoint("newJSonConfigData", reasonFails);
	if (l_newJSonConfigData == nullptr) return false;

	l_addStringData = (addStringDataSignature)GetLogManagerLibraryEntryPoint("addStringData", reasonFails);
	if (l_addStringData == nullptr) return false;

	l_addIntData = (addIntDataSignature)GetLogManagerLibraryEntryPoint("addIntData", reasonFails);
	if (l_addIntData == nullptr) return false;

	l_addFloatData = (addFloatDataSignature)GetLogManagerLibraryEntryPoint("addFloatData", reasonFails);
	if (l_addFloatData == nullptr) return false;

	l_addBoolData = (addBoolDataSignature)GetLogManagerLibraryEntryPoint("addBoolData", reasonFails);
	if (l_addBoolData == nullptr) return false;

	l_addUIntData = (addUIntDataSignature)GetLogManagerLibraryEntryPoint("addUIntData", reasonFails);
	if (l_addUIntData == nullptr) return false;

	l_addLongLongData = (addLongLongDataSignature)GetLogManagerLibraryEntryPoint("addLongLongData", reasonFails);
	if (l_addLongLongData == nullptr) return false;

	l_addULongLongData = (addULongLongDataSignature)GetLogManagerLibraryEntryPoint("addULongLongData", reasonFails);
	if (l_addULongLongData == nullptr) return false;

	l_addComposedData = (addComposedDataSignature)GetLogManagerLibraryEntryPoint("addComposedData", reasonFails);
	if (l_addComposedData == nullptr) return false;

	l_getNextID = (getNextIDSignature)GetLogManagerLibraryEntryPoint("getNextID", reasonFails);
	if (l_getNextID == nullptr) return false;

	//Start the logger
	char* error = l_initLogger();
	if (error != nullptr) {
		reasonFails = error;
		return false;
	}

	UE_LOG(LogManager, Log, TEXT("Initialize DLL OK :)\n"));
	return true;
}

void FLogManagerModule::Shutdown()
{
	// This function may be called during shutdown to clean up your module.
	// For modules that support dynamic reloading,
	// we call this function before unloading the module.
	l_shutdownLogger();
	// Free the dll handle
	FPlatformProcess::FreeDllHandle(logManagerLibraryHandle);
	logManagerLibraryHandle = nullptr;
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
	return l_getNextID(OriginalId);
}

// ----------------------------------------------------------------------------------------
// Open/Close a log file
// ----------------------------------------------------------------------------------------

bool FLogManagerModule::OpenNewJSonFile(FString& filepath)
{
	const char* error = l_openNewJSonFile(TCHAR_TO_ANSI(*filepath));
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("Opening %s to write fail: %s"), *filepath, UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::FlushAndCloseJSonFile()
{
	const char* error = l_flushAndCloseJSonFile();
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("Close current file fail : %s\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	UE_LOG(LogManager, Log, TEXT("Close file :)\n"));
	return true;
}

// ----------------------------------------------------------------------------------------
// Begin/End a log entry
// ----------------------------------------------------------------------------------------

bool FLogManagerModule::BeginLogEntry(EEvent ename, float gameTime, int nbFields)
{
	// Try to lock the buffer to write a log entry
	char* error = l_lockWrite();
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("BeginLogEntry fail lockWrite: %s\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	// Register a log entry for the current event
	// Capture the current wall-clock timestamp alongside the in-game time
	FDateTime date = FDateTime::Now();
	FString EventStr = eventToString(ename);
	FTCHARToUTF8 Converted(*EventStr); // persistant convertion
	const char* EventAnsi = Converted.Get();

	error = l_newJSonEvent(EventAnsi, gameTime, date.ToUnixTimestamp(), date.GetMillisecond(), (uint64)GFrameNumber, nbFields);

	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("BeginLogEntry fail newJSonEvent: %s\n"), UTF8_TO_TCHAR(error));
		// Release the lock even on failure to avoid deadlocking the write buffer
		FinalizeConfigOrLogEntry();
		UE_LOG(LogManager, Error, TEXT("BeginLogEntry: unlock Write\n"));
		return false;
	}
	UE_LOG(LogManager, Log, TEXT("Write new JSonEvent %s !\n"), *eventToString(ename));
	return true;
}

bool FLogManagerModule::BeginConfigData(int nbFields) {
	// Try to lock the buffer to write a log entry
	char* error = l_lockWrite();
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("BeginConfigData fail lockWrite: %s\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	error = l_newJSonConfigData(nbFields);

	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("BeginConfigData fail newJSonConfigData: %s\n"), UTF8_TO_TCHAR(error));
		// Release the lock even on failure to avoid deadlocking the write buffer
		FinalizeConfigOrLogEntry();
		UE_LOG(LogManager, Error, TEXT("BeginConfigData: unlock Write\n"));
		return false;
	}
	UE_LOG(LogManager, Log, TEXT("Start to write new ConfigData (%d)!\n"), nbFields);
	return true;
}

bool FLogManagerModule::FinalizeConfigOrLogEntry()
{
	char* error = l_unlockWrite();
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("FinalizeConfigOrLogEntry: fail to unlock Write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Add data to a log entry
// ----------------------------------------------------------------------------------------

bool FLogManagerModule::AddStringData(const FString key, const FString& value)
{
	char* error = l_addStringData(TCHAR_TO_ANSI(*key), TCHAR_TO_ANSI(*value));
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddStringData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddIntData(const FString& key, int value)
{
	char* error = l_addIntData(TCHAR_TO_ANSI(*key), value);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddIntData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddFloatData(const FString& key, float value)
{
	char* error = l_addFloatData(TCHAR_TO_ANSI(*key), value);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddFloatData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddBoolData(const FString& key, bool value)
{
	char* error = l_addBoolData(TCHAR_TO_ANSI(*key), value);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddBoolData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddUIntData(const FString& key, unsigned int value)
{
	char* error = l_addUIntData(TCHAR_TO_ANSI(*key), value);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddUIntData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddLongLongData(const FString& key, signed long long value)
{
	char* error = l_addLongLongData(TCHAR_TO_ANSI(*key), value);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddLongLongData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddULongLongData(const FString& key, unsigned long long value)
{
	char* error = l_addULongLongData(TCHAR_TO_ANSI(*key), value);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddULongLongData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
		return false;
	}
	return true;
}

bool FLogManagerModule::AddComposedData(const FString& key, unsigned int nbSubPairs)
{
	char* error = l_addComposedData(TCHAR_TO_ANSI(*key), nbSubPairs);
	if (error != nullptr) {
		UE_LOG(LogManager, Error, TEXT("AddComposedData: fail to write (%s)\n"), UTF8_TO_TCHAR(error));
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
		UE_LOG(LogManager, Error, TEXT("LoadConfigJson: the file (%s) does not exist\n"), *filePath);
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
		UE_LOG(LogManager, Error, TEXT("LoadConfigJson: fail to read file content after %d tries (%s)\n"), maxRetries, *filePath);
		return nullptr;
	}
	else if (fileContent == "") {
		UE_LOG(LogManager, Error, TEXT("LoadConfigJson: fail to get file content or file is empty (%s)\n"), *filePath);
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
	UE_LOG(LogManager, Error, TEXT("Failed to deserialize JSON data: %s"), *filePath);
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
