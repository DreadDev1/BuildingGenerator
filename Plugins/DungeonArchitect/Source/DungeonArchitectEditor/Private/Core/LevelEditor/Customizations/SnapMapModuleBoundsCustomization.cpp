//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Customizations/SnapMapModuleBoundsCustomization.h"

#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Frameworks/Snap/Lib/Module/SnapModuleBoundShape.h"

#include "DetailLayoutBuilder.h"
#include "EdMode.h"

void FSnapMapModuleBoundsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) {
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Dungeon Architect", FText::GetEmpty(), ECategoryPriority::Important);

	auto SetPropertyVisible = [&](const FName& InPropertyPath, bool bVisible) {
		if (bVisible) {
			const TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(InPropertyPath);
			if (FProperty* Property = PropertyHandle->GetProperty()) {
				Property->SetMetaData(FEdMode::MD_MakeEditWidget, TEXT("true"));
			}
		}
		else {
			const TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(InPropertyPath);
			PropertyHandle->MarkHiddenByCustomization();
			if (FProperty* Property = PropertyHandle->GetProperty()) {
				Property->RemoveMetaData(FEdMode::MD_MakeEditWidget);
			}
		}
	};

	if (const ASnapModuleBoundShape* BoundsActor = FDungeonEditorUtils::GetBuilderObject<ASnapModuleBoundShape>(&DetailBuilder)) {
		SetPropertyVisible(GET_MEMBER_NAME_CHECKED(ASnapModuleBoundShape, BoxExtent), BoundsActor->BoundsType == EDAShapePolygonType::Box);
		SetPropertyVisible(GET_MEMBER_NAME_CHECKED(ASnapModuleBoundShape, CircleRadius), BoundsActor->BoundsType == EDAShapePolygonType::Circle);
		SetPropertyVisible(GET_MEMBER_NAME_CHECKED(ASnapModuleBoundShape, PolygonPoints), BoundsActor->BoundsType == EDAShapePolygonType::Polygon);
	}

}

TSharedRef<IDetailCustomization> FSnapMapModuleBoundsCustomization::MakeInstance() {
	return MakeShareable(new FSnapMapModuleBoundsCustomization);
}



