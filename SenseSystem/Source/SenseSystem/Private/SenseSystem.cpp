//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseSystem.h"

#define LOCTEXT_NAMESPACE "FSenseSystemModule"

void FSenseSystemModule::StartupModule()
{
}

void FSenseSystemModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSenseSystemModule, SenseSystem)
DEFINE_LOG_CATEGORY(LogSenseSys);