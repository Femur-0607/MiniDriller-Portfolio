#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleHUDWidget.generated.h"

UCLASS()
class MINIDRILLER_API UTitleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 에디터에서 바인딩할 버튼 2개
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Btn_Start;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* Btn_Quit;

	// 버튼 클릭 이벤트 함수
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnQuitClicked();
	
	// 최고 기록을 표시할 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* Txt_MaxDepth;

	// 획득한 뱃지 개수를 표시할 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* Txt_Badge;
};