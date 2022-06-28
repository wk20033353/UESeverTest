// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

/*
 * Interface for the Able Editor.
 */
class IAbleEditor : public IModuleInterface
{

public:

	/**
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IAbleEditor& Get()
	{
		return FModuleManager::LoadModuleChecked< IAbleEditor >( "AbleEditor" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "AbleEditor" );
	}

	/* Returns the Able Category for Able specific assets. */
	virtual uint32 GetAbleAssetCategory() const = 0;
};

