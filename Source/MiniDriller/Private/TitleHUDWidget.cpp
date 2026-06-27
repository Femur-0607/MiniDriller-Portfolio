#include "TitleHUDWidget.h"

#include "GameStatusSubsystem.h"
#include "MiniDrillerSaveGame.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UTitleHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 이벤트 연결
	if (Btn_Start)
	{
		Btn_Start->OnClicked.AddDynamic(this, &UTitleHUDWidget::OnStartClicked);
	}
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddDynamic(this, &UTitleHUDWidget::OnQuitClicked);
	}
	
	// 2. NativeConstruct() 함수의 가장 아랫부분에 추가해 주세요!
	if (UGameStatusSubsystem* statusSystem = GetWorld()->GetSubsystem<UGameStatusSubsystem>())
	{
		// 혹시 게임 켜자마자 데이터가 안 불러와졌을까봐 UI 띄울 때 확실하게 로드!
		statusSystem->LoadGameData();

		// 세이브 데이터 그릇이 정상적으로 준비되었다면 텍스트를 갱신합니다.
		if (statusSystem->currentSaveData)
		{
			if (Txt_MaxDepth)
			{
				// C++의 printf 문법을 사용해 문자열을 깔끔하게 조립합니다.
				FString DepthStr = FString::Printf(TEXT("MAX DEPTH : %d M"), statusSystem->currentSaveData->maxDepth);
				Txt_MaxDepth->SetText(FText::FromString(DepthStr));
			}

			if (Txt_Badge)
			{
				FString BadgeStr = FString::Printf(TEXT("BADGES : %d / 10"), statusSystem->currentSaveData->unlockedBadgeCount);
				Txt_Badge->SetText(FText::FromString(BadgeStr));
			}
		}
	}
}

void UTitleHUDWidget::OnStartClicked()
{
	UGameplayStatics::OpenLevel(this, FName("L_InGame"));
}

void UTitleHUDWidget::OnQuitClicked()
{
	// 게임 완전히 종료하기 (에디터에서는 플레이가 멈추고, 빌드 파일에서는 창이 꺼집니다)
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, true);
}