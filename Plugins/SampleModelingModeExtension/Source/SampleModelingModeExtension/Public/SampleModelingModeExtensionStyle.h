// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"

class SAMPLEMODELINGMODEEXTENSION_API FSampleModelingModeExtensionStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static TSharedPtr<class ISlateStyle> Get();
	static FName GetStyleSetName();

private:
	static FString InContent(const FString& RelativePath, const ANSICHAR* Extension);
	static TSharedPtr<class FSlateStyleSet> StyleSet;
};