#pragma once

extern "C" {
	// ----------------------------------------------------------------------------------------
	// All the DLL function signatures and entry points
	// ----------------------------------------------------------------------------------------

	typedef char*     (*initLoggerSignature)();
	typedef void      (*shutdownLoggerSignature)();
	typedef char*     (*openNewJSonFileSignature)(const char* filepath);
	typedef char*     (*flushAndCloseJSonFileSignature)();
	typedef char*     (*lockWriteSignature)();
	typedef char*     (*unlockWriteSignature)();
	typedef char*     (*newJSonEventSignature)(const char* e, float gameTime, signed long long unixTimeSeconds, int unixTimeMilliseconds, signed long long frameCount, unsigned int nbDatas);
	typedef char*     (*newJSonConfigData)(unsigned int nbDatas);
	typedef char*     (*addStringDataSignature)(const char* key, const char* value);
	typedef char*     (*addIntDataSignature)(const char* key, int value);
	typedef char*     (*addFloatDataSignature)(const char* key, float value);
	typedef char*     (*addBoolDataSignature)(const char* key, bool value);
	typedef char*     (*addUIntDataSignature)(const char* key, unsigned int value);
	typedef char*     (*addLongLongDataSignature)(const char* key, signed long long value);
	typedef char*     (*addULongLongDataSignature)(const char* key, unsigned long long value);
	typedef char*     (*addComposedDataSignature)(const char* key, unsigned int nbSubPairs);
	typedef long long (*getNextIDSignature)(long long OriginalID);
	
	initLoggerSignature             l_initLogger = nullptr;
	shutdownLoggerSignature         l_shutdownLogger = nullptr;
	openNewJSonFileSignature        l_openNewJSonFile = nullptr;
	flushAndCloseJSonFileSignature  l_flushAndCloseJSonFile = nullptr;
	lockWriteSignature              l_lockWrite = nullptr;
	unlockWriteSignature            l_unlockWrite = nullptr;
	newJSonEventSignature           l_newJSonEvent = nullptr;
	newJSonConfigData               l_newJSonConfigData = nullptr;
	addStringDataSignature          l_addStringData = nullptr;
	addIntDataSignature             l_addIntData = nullptr;
	addFloatDataSignature           l_addFloatData = nullptr;
	addBoolDataSignature            l_addBoolData = nullptr;
	addUIntDataSignature            l_addUIntData = nullptr;
	addLongLongDataSignature        l_addLongLongData = nullptr;
	addULongLongDataSignature       l_addULongLongData = nullptr;
	addComposedDataSignature        l_addComposedData = nullptr;
	getNextIDSignature              l_getNextID = nullptr;
}