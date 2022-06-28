// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/AblAbilityThumbnailRenderer.h"

#include "AbleEditorPrivate.h"

#include "ablAbility.h"
#include "ablAbilityBlueprint.h"
#include "CanvasTypes.h"
#include "EngineModule.h"
#include "Engine/Texture2D.h"
#include "SceneView.h"
#include "UObject/ConstructorHelpers.h"

UAblAbilityThumbnailRenderer::UAblAbilityThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_NoImage(nullptr)
{
	// Borrow the Particle System's texture for as a default image.
	ConstructorHelpers::FObjectFinder<UTexture2D> PSysThumbnail_NoImage(TEXT("/Engine/EditorMaterials/ParticleSystems/PSysThumbnail_NoImage"));
	m_NoImage = PSysThumbnail_NoImage.Object;
}

void UAblAbilityThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UAblAbility* Ability = Cast<UAblAbility>(Object);
	if (!Ability)
	{
		if (UAblAbilityBlueprint* AbilityBlueprint = Cast<UAblAbilityBlueprint>(Object))
		{
			if (AbilityBlueprint->GeneratedClass)
			{
				Ability = Cast<UAblAbility>(AbilityBlueprint->GeneratedClass->GetDefaultObject());
			}
		}
	}

	if (Ability)
	{
		if (Ability->ThumbnailImage)
		{
			if (!Ability->ThumbnailImage->Resource)
			{
				Ability->ThumbnailImage->UpdateResource();
			}

			if (Ability->ThumbnailImage->Resource)
			{
				Canvas->DrawTile(X, Y, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, Ability->ThumbnailImage->Resource, false);
			}
		}
		else if (m_NoImage)
		{
			Canvas->DrawTile(X, Y, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, m_NoImage->Resource, false);
		}
	}
}
