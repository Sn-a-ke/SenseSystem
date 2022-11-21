//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Types/SlateEnums.h"
#include "IPropertyTypeCustomization.h"
#include "SenseSysHelpers.h"

/*
#include "Misc/Optional.h"
#include "Misc/Attribute.h"
#include "Fonts/SlateFontInfo.h"
*/

class SENSESYSTEMEDITOR_API SBitMask64Widget : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnValueChanged, uint64);
	DECLARE_DELEGATE_TwoParams(FOnValueCommitted, uint64, ETextCommit::Type);

	SLATE_BEGIN_ARGS(SBitMask64Widget) {}
	SLATE_ARGUMENT(uint64, InBitMask)
	//SLATE_ATTRIBUTE(FSlateFontInfo, Font)
	SLATE_EVENT(FOnValueChanged, OnValueChanged)
	SLATE_EVENT(FOnValueCommitted, OnValueCommitted)
	SLATE_END_ARGS()

	SBitMask64Widget() {}
	~SBitMask64Widget() {}

	void Construct(const FArguments& InArgs);

	uint64 BitMask = 0;
	FOnValueChanged ValueChanged;
	FOnValueCommitted ValueCommitted;

	uint64 OnGetValue() const { return BitMask; }
	void OnValueCommitted(uint64 NewValue, ETextCommit::Type CommitInfo);
	void OnValueChanged(uint64 NewValue);

private:
	TSharedPtr<class SWidget> PrimaryWidget;

	struct FBitmaskFlagInfo
	{
		uint64 Value;
		FText DisplayName;
		FText ToolTipText;
	};
};

class IPropertyHandle;

/**
 *
 */
class SENSESYSTEMEDITOR_API FBitFlag64_Customization : public IPropertyTypeCustomization //, public FEditorUndoClient
{
public:
	FBitFlag64_Customization();

	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FBitFlag64_Customization()); }

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	void OnValueChanged(uint64 Val);
	void OnValueCommitted(uint64 Val, ETextCommit::Type CommitInfo);

	FBitFlag64_SenseSys GT;
	FBitFlag64_SenseSys FromProperty() const;

private:
	TSharedPtr<IPropertyHandle> PropertyHandle;
};
