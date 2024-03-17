//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BitFlag64_Customization.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Engine/GameViewportClient.h"
#include "PropertyHandle.h"
#include "IPropertyTypeCustomization.h"

#include "Delegates/DelegateSignatureImpl.inl"
#include "DetailWidgetRow.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/STextBlock.h"

#include "Textures/SlateIcon.h"

//#include "EditorStyleSet.h"
//#include "Framework/Commands/UIAction.h"
//#include "Widgets/Input/NumericTypeInterface.h"


#define LOCTEXT_NAMESPACE "PropertyEditor"

void SBitMask64Widget::Construct(const FArguments& InArgs)
{
	BitMask = InArgs._InBitMask;
	ValueChanged = InArgs._OnValueChanged;
	ValueCommitted = InArgs._OnValueCommitted;

	auto CreateBitmaskFlagsArray = []() -> TStaticArray<FBitmaskFlagInfo, 64>
		{
			TStaticArray<FBitmaskFlagInfo, 64> Result;
			for (int32 Idx = 0; Idx < 64; Idx++)
			{
				Result[Idx].Value = 1llu << Idx;
				Result[Idx].DisplayName = FText::AsNumber(Idx + 1);
				Result[Idx].ToolTipText = FText::Format(LOCTEXT("BitmaskDefaultFlagToolTipText", "Channel {0} on/off"), Result[Idx].DisplayName);
			}
			return Result;
		};

	const auto& GetComboButtonText = [this, CreateBitmaskFlagsArray]() -> FText
		{
			const int64 BitmaskValue = OnGetValue();
			if (BitmaskValue != 0)
			{
				const TStaticArray<FBitmaskFlagInfo, 64> BitmaskFlags = CreateBitmaskFlagsArray();
				FString Out;

				for (int32 Idx = 0; Idx < 64; Idx++)
				{
					if (BitmaskValue & BitmaskFlags[Idx].Value)
					{
						const FString SText = Out.Len() == 0 ? FString::Printf(TEXT("%s"), *BitmaskFlags[Idx].DisplayName.ToString())
															 : FString::Printf(TEXT(", %s"), *BitmaskFlags[Idx].DisplayName.ToString());
						Out.Append(SText);
					}
				}
				return FText::FromString(Out);
			}
			return LOCTEXT("BitmaskButtonContentNoFlagsSet", "None");
		};

	const FComboBoxStyle& ComboBoxStyle = FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox");

	SAssignNew(PrimaryWidget, SComboButton)
		.ComboButtonStyle(&ComboBoxStyle.ComboButtonStyle)
		.ContentPadding(FMargin(4.0, 2.0))
		.ButtonContent()[SNew(STextBlock).Text_Lambda(GetComboButtonText)]
		.OnGetMenuContent_Lambda([this, CreateBitmaskFlagsArray]() 
			{
				FMenuBuilder MenuBuilder(false, nullptr);
				TStaticArray<FBitmaskFlagInfo, 64> BitmaskFlags = CreateBitmaskFlagsArray();
				for (int32 i = 0; i < BitmaskFlags.Num(); i++)
				{
					MenuBuilder.AddMenuEntry(
						BitmaskFlags[i].DisplayName,
						BitmaskFlags[i].ToolTipText,
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda(
								[this, i, BitmaskFlags]() { OnValueCommitted((OnGetValue() ^ BitmaskFlags[i].Value), ETextCommit::Default); }),
							FCanExecuteAction(),
							FIsActionChecked::CreateLambda(
								[this, i, BitmaskFlags]() -> bool { return (OnGetValue() & BitmaskFlags[i].Value) != static_cast<uint64>(0); })),
						NAME_None,
						EUserInterfaceActionType::Check);
				}
				return MenuBuilder.MakeWidget();
			});

	ChildSlot.AttachWidget(PrimaryWidget.ToSharedRef());
}


void SBitMask64Widget::OnValueCommitted(const uint64 NewValue, const ETextCommit::Type CommitInfo)
{
	BitMask = NewValue;
	ValueCommitted.ExecuteIfBound(BitMask, CommitInfo);
}

void SBitMask64Widget::OnValueChanged(const uint64 NewValue)
{
	BitMask = NewValue;
	ValueChanged.ExecuteIfBound(BitMask);
}

#undef LOCTEXT_NAMESPACE

void FBitFlag64_Customization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;
	GT = FromProperty();
	HeaderRow.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()].ValueContent().MaxDesiredWidth(0.0f).MinDesiredWidth(
		512.f)[SNew(SBitMask64Widget)
				   .InBitMask(GT.Value)
				   .OnValueChanged(this, &FBitFlag64_Customization::OnValueChanged)
				   .OnValueCommitted(this, &FBitFlag64_Customization::OnValueCommitted)];
}

void FBitFlag64_Customization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FBitFlag64_Customization::OnValueChanged(const uint64 Val)
{
	GT.Value = Val;
}

void FBitFlag64_Customization::OnValueCommitted(const uint64 Val, ETextCommit::Type CommitInfo)
{
	GT.Value = Val;
	if (PropertyHandle.IsValid())
	{
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);
		for (void* RawDataInstance : RawData)
		{
			FBitFlag64_SenseSys& Ref = *(FBitFlag64_SenseSys*)(RawDataInstance) = GT;
			Ref = GT;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SenseSys FBitFlag64_Customization::OnValueCommitted PropertyHandle.IsValid() FAILED!"));
	}
}

FBitFlag64_SenseSys FBitFlag64_Customization::FromProperty() const
{
	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);

	if (RawData.Num() != 1)
	{
		return FBitFlag64_SenseSys();
	}
	FBitFlag64_SenseSys* DataPtr = (FBitFlag64_SenseSys*)(RawData[0]);
	if (DataPtr == nullptr)
	{
		return FBitFlag64_SenseSys();
	}
	return *DataPtr;
}

FBitFlag64_Customization::FBitFlag64_Customization()
{
}
