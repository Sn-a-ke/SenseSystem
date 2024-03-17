//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FSenseSystemModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	using ElementIndexType = uint16; //todo options
};

DECLARE_LOG_CATEGORY_EXTERN(LogSenseSys, Log, All); //Fatal, Error, Warning, Display, Log, Verbose, VeryVerbose