//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseSystem.h"

#define LOCTEXT_NAMESPACE "FSenseSystemModule"

void FSenseSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FSenseSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSenseSystemModule, SenseSystem)
DEFINE_LOG_CATEGORY(LogSenseSys);