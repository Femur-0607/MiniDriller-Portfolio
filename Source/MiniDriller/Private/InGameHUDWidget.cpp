// 유지보수와 퍼포먼스를 극대화한 하이브리드 인게임 HUD 위젯 구현부

#include "InGameHUDWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "GameStatusSubsystem.h"
#include "PaperSprite.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UInGameHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 서브시스템 캐싱
	statusSubsystemRef = GetWorld()->GetSubsystem<UGameStatusSubsystem>();
	
	// 1. 게임 시작 시 결과 창 숨기기
	if (GameOverGroup)
	{
		GameOverGroup->SetVisibility(ESlateVisibility::Hidden);
	}

	// 2. 버튼 클릭 이벤트 연결
	if (Btn_Restart)
	{
		// AddDynamic은 언리얼 엔진(Unreal Engine) C++에서 다이내믹 멀티캐스트 델리게이트(Dynamic Multicast Delegate)에 콜백 함수를 바인딩할 때 사용하는 매크로입니다.
		Btn_Restart->OnClicked.AddDynamic(this, &UInGameHUDWidget::OnRestartClicked);
	}
	
	if (Btn_Title)
	{
		Btn_Title->OnClicked.AddDynamic(this, &UInGameHUDWidget::OnTitleClicked);
	}

	// 3. 서브시스템의 죽음 방송국(OnPlayerDied)에 내 귀(HandlePlayerDeath)를 연결!
	if (statusSubsystemRef)
	{
		statusSubsystemRef->OnPlayerDied.AddDynamic(this, &UInGameHUDWidget::HandlePlayerDeath);
	}

	// [가장 완벽한 방식] 코드로 명시된 머티리얼을 직접 사용하여 MID 생성
	if (OxygenBaseMaterial && OxygenImage)
	{
		// 1. 변수에 할당된 원본 머티리얼로 동적 머티리얼 생성
		oxygenDynamicMaterial = UMaterialInstanceDynamic::Create(OxygenBaseMaterial, this);
		
		// 2. 이미지 위젯에 강제로 덮어씌우기
		OxygenImage->SetBrushFromMaterial(oxygenDynamicMaterial);
	}
	
	// [추가] 게임 시작 사운드 재생
    if (gameStartSound)
    {
        UGameplayStatics::PlaySound2D(this, gameStartSound);
    }
	
	// Game Start 애니메이션 재생
	if (Anim_GameStart)
	{
		PlayAnimation(Anim_GameStart);
	}
}

void UInGameHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (statusSubsystemRef)
	{
		// 1. 산소가 바뀌었을 때만 갱신 (오차 범위 0.1 허용)
		if (!FMath::IsNearlyEqual(lastOxygen, statusSubsystemRef->currentOxygen, 0.1f))
		{
			UpdateOxygenUI(statusSubsystemRef->currentOxygen);
			lastOxygen = statusSubsystemRef->currentOxygen;
		}

		// 2. 깊이가 바뀌었을 때만 갱신
		if (lastDepth != statusSubsystemRef->currentDepth)
		{
			UpdateNumberUI(statusSubsystemRef->currentDepth, HB_Depth);
			lastDepth = statusSubsystemRef->currentDepth;
		}

		// 3. 점수가 바뀌었을 때만 갱신
		if (lastScore != statusSubsystemRef->currentScore)
		{
			UpdateNumberUI(statusSubsystemRef->currentScore, HB_Score);
			lastScore = statusSubsystemRef->currentScore;
		}

		// 4. 레벨이 바뀌었을 때만 갱신
		if (lastLevel != statusSubsystemRef->currentLevel)
		{
			UpdateNumberUI(statusSubsystemRef->currentLevel, HB_Level);
			if (lastLevel > 0 && Anim_LevelUp)
			{
				PlayAnimation(Anim_LevelUp);
			}
			lastLevel = statusSubsystemRef->currentLevel;
		}
	}
}

void UInGameHUDWidget::HandlePlayerDeath()
{
	if (GameOverGroup)
	{
		if (statusSubsystemRef)
		{
			// 1. 브금 스르륵 끄기
			statusSubsystemRef->StopBGM(1.0f);
			
			// 🛠️ [타이밍 버그 해결] 결과창에 글자를 채우기 전에 장부를 마감하고 디스크에 구워버립니다!
			// 이 안에서 bIsNewRecord가 실시간으로 계산됩니다.
			statusSubsystemRef->SaveGameData();

			// 2. 결과창 텍스트 갱신
			if (Txt_ResultDepth) Txt_ResultDepth->SetText(FText::FromString(FString::Printf(TEXT("%d M"), statusSubsystemRef->currentDepth)));
			if (Txt_ResultScore) Txt_ResultScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), statusSubsystemRef->currentScore)));
			if (Txt_ResultLevel) Txt_ResultLevel->SetText(FText::FromString(FString::Printf(TEXT("LV. %d"), statusSubsystemRef->currentLevel)));
			
			// 🌟 [게임 오버 사운드 재생]
			if (GameOverSound)
			{
				UGameplayStatics::PlaySound2D(this, GameOverSound);
			}

			// 3. 대망의 최고 기록 연출!
			if (Img_NewRecord)
			{
				if (statusSubsystemRef->bIsNewRecord)
				{
					Img_NewRecord->SetVisibility(ESlateVisibility::Visible);
					// (선택) 여기에 팡파레 사운드를 추가하면 완벽합니다!
				}
				else
				{
					Img_NewRecord->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
		
		// 1. 숨겨놨던 결과 창을 화면에 띄웁니다.
		GameOverGroup->SetVisibility(ESlateVisibility::Visible);

		// 2. 마우스 커서를 켜고, UI만 클릭할 수 있게 입력 모드를 바꿉니다.
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			PC->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
			PC->SetInputMode(InputMode);
		}
	}
}

void UInGameHUDWidget::OnRestartClicked()
{
	// 다음 맵으로 가기 전 플레이어의 조작 권한을 돌려줍니다.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	FString CurrentLevelName = GetWorld()->GetName();
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}

void UInGameHUDWidget::OnTitleClicked()
{
	if (statusSubsystemRef)
	{
		// 1. 인게임 브금을 부드럽게 끕니다. (타이틀로 넘어가면 타이틀 레벨BP가 알아서 자기 브금을 틉니다!)
		statusSubsystemRef->StopBGM(1.0f);
	}
	
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	// 2. 타이틀 맵으로 이동합니다! 
	// (주의: 맵 파일 이름이 정확히 "L_Title"인지 확인해 주세요. 다르면 그 이름으로 적으시면 됩니다.)
	UGameplayStatics::OpenLevel(this, FName("L_Title"));
}

void UInGameHUDWidget::UpdateNumberUI(int32 Value, UHorizontalBox* TargetBox)
{
	if (TargetBox == nullptr || NumberSprites.Num() < 10) return;

	int32 TempCalc = Value;
	int32 ChildCount = TargetBox->GetChildrenCount();

	// [역순 루프 알고리즘] 10으로 나눈 나머지(일의 자리)가 먼저 나오므로, 수평 박스의 가장 오른쪽 자식부터 역순으로 채웁니다.
	for (int32 i = ChildCount - 1; i >= 0; --i)
	{
		UWidget* ChildWidget = TargetBox->GetChildAt(i);
		UImage* DigitImage = Cast<UImage>(ChildWidget);

		if (DigitImage)
		{
			// 현재 자릿수의 숫자 추출 (0 ~ 9)
			int32 Digit = TempCalc % 10;

			// 안전검사 후 이미지 교체 (UImage에 스프라이트를 씌우는 언리얼 C++ 공식 함수)
			if (NumberSprites.IsValidIndex(Digit))
			{
				DigitImage->SetBrushResourceObject(NumberSprites[Digit]);
			}

			// 다음 자릿수 계산을 위해 10으로 나눈 몫을 취함 (예: 153 -> 15 -> 1)
			// 만약 숫자가 다 떨어져서 TempCalc가 0이 되어도, 남은 앞자리 이미지들은 자동으로 '0' 스프라이트로 채워집니다! (0053 효과)
			TempCalc /= 10;
		}
	}
}

void UInGameHUDWidget::UpdateOxygenUI(float CurrentOxygen)
{
	if (!GEngine) return;
	
	// 게이지바 갱신
	if (oxygenDynamicMaterial)
	{
		float Percent = CurrentOxygen / 100.0f;
		
		oxygenDynamicMaterial->SetScalarParameterValue(FName("oxygenPercent"), Percent);
	}
	
	// 산소 % 텍스트 갱신
	if (Txt_Oxygen)
	{
		// FMath::RoundToInt를 써서 99.8f 같은 소수점을 깔끔한 정수(100)로 반올림합니다.
		int32 DisplayOxygen = FMath::RoundToInt(CurrentOxygen);

		// FString::Printf에서 '%' 기호를 출력하려면 '%%'라고 두 번 적어야 합니다!
		FString OxygenString = FString::Printf(TEXT("%d %%"), DisplayOxygen);
		
		Txt_Oxygen->SetText(FText::FromString(OxygenString));
	}
}





