// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "AbilityEditor/SAbilitySelectPreviewAssetDlg.h"

#include "AbleEditorPrivate.h"

#include "AbilityEditor/AblAbilityEditorSettings.h"
#include "AbilityEditor/SAbilityPreviewAssetClassDlg.h"
#include "AssetRegistryModule.h"
#include "Animation/AnimBlueprint.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AblAbilityEditor"


void SAblAbilitySelectPreviewAssetDlg::Construct(const FArguments& InArgs)
{
	m_bOkClicked = false;
	m_ModalWindow = nullptr;

	ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
			.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
			[
				SNew(SBox)
				.Visibility(EVisibility::Visible)
				.WidthOverride(500.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Content()
						[
							SAssignNew(m_AssetPickerContainer, SVerticalBox)
						]
					]

					// Ok/Cancel buttons
					+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Bottom)
						.Padding(8)
						[
							SNew(SUniformGridPanel)
							.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
							.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
							.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
							+ SUniformGridPanel::Slot(0, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
								.OnClicked(this, &SAblAbilitySelectPreviewAssetDlg::OkClicked)
								.Text(LOCTEXT("AblAbilityAnimationSelectorOK", "OK"))
							]
						+ SUniformGridPanel::Slot(1, 0)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
								.OnClicked(this, &SAblAbilitySelectPreviewAssetDlg::CancelClicked)
								.Text(LOCTEXT("AblAbilityAnimationSelectorCancel", "Cancel"))
							]
						]
				]
			]
		];

	MakePicker();
}

bool SAblAbilitySelectPreviewAssetDlg::DoModal()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("AblAbilityPreviewAssetPicker", "Select Asset"))
		.ClientSize(FVector2D(400, 700))
		.SupportsMinimize(false).SupportsMaximize(false)
		[
			AsShared()
		];

	m_ModalWindow = Window;

	GEditor->EditorAddModalWindow(Window);

	return m_bOkClicked;
}

void SAblAbilitySelectPreviewAssetDlg::MakePicker()
{
	// Query for the class to look for first...
	TSharedRef<SAblAbilityPreviewAssetClassDlg> AssetClassDialog = SNew(SAblAbilityPreviewAssetClassDlg);
	AssetClassDialog->DoModal();

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	const UAblAbilityEditorSettings* EditorSettings = GetDefault<UAblAbilityEditorSettings>();
	check(EditorSettings);

	// Configure filter for asset picker
	FAssetPickerConfig Config;
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAblAbilitySelectPreviewAssetDlg::OnAssetSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAblAbilitySelectPreviewAssetDlg::OnAssetDoubleClicked);

	if (const UClass* SelectedClass = AssetClassDialog->GetSelectedClass())
	{
		// User selected a specific class.
		Config.Filter.ClassNames.Add(SelectedClass->GetFName());
	}
	else
	{
		if (EditorSettings->m_AllowStaticMeshes)
		{
			Config.Filter.ClassNames.Add(UStaticMesh::StaticClass()->GetFName());
		}

		if (EditorSettings->m_AllowSkeletalMeshes)
		{
			Config.Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());
		}

		if (EditorSettings->m_AllowAnimationBlueprints)
		{
			Config.Filter.ClassNames.Add(UAnimBlueprint::StaticClass()->GetFName());
		}

		if (EditorSettings->m_AllowPawnBlueprints)
		{
			Config.Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
			Config.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SAblAbilitySelectPreviewAssetDlg::OnShouldFilterAsset);
		}
	}


	Config.bCanShowFolders = true;
	Config.bPreloadAssetsForContextMenu = true;

	m_AssetPickerContainer->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectAnimationAsset", "Select Asset:"))
		.ShadowOffset(FVector2D(1.0f, 1.0f))
		];
	m_AssetPickerContainer->AddSlot()
		[
			ContentBrowserModule.Get().CreateAssetPicker(Config)
		];
}

void SAblAbilitySelectPreviewAssetDlg::OnAssetSelected(const FAssetData& AssetData)
{
	m_Asset = AssetData.GetAsset();
}

void SAblAbilitySelectPreviewAssetDlg::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	m_Asset = AssetData.GetAsset();
	m_bOkClicked = true;
	CloseDialog();
}

bool SAblAbilitySelectPreviewAssetDlg::OnShouldFilterAsset(const FAssetData& AssetData) const
{
	if (const UClass* AssetClass = AssetData.GetClass())
	{
		if (AssetClass->IsChildOf(UBlueprint::StaticClass()))
		{
			if (AssetClass->IsChildOf(UAnimBlueprint::StaticClass()))
			{
				return false;
			}

			if (const UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset()))
			{
				if (BlueprintAsset->GeneratedClass)
				{
					if (BlueprintAsset->GeneratedClass->IsChildOf(APawn::StaticClass()))
					{
						return false;
					}
				}
			}

			return true;
		}
	}
	return false;
}

FReply SAblAbilitySelectPreviewAssetDlg::OkClicked()
{
	m_bOkClicked = true;

	CloseDialog();

	return FReply::Handled();
}

void SAblAbilitySelectPreviewAssetDlg::CloseDialog()
{
	if (m_ModalWindow.IsValid())
	{
		m_ModalWindow.Pin()->RequestDestroyWindow();
	}
}

FReply SAblAbilitySelectPreviewAssetDlg::CancelClicked()
{
	m_bOkClicked = false;

	CloseDialog();

	return FReply::Handled();
}

FReply SAblAbilitySelectPreviewAssetDlg::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		m_bOkClicked = false;

		CloseDialog();

		return FReply::Handled();
	}

	return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE