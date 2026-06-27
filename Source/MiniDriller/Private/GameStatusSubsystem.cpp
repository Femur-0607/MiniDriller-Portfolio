// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStatusSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrillerCharacter.h"
#include "MiniDrillerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

void UGameStatusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	LoadGameData();

	/// 1초마다 DecreaseOxygenTick 함수를 반복 실행하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		oxygenTimerHandle,                           // 조종할 핸들 변수 (lowerCamelCase 적용)
		this,                                        // 함수를 보유한 객체 (서브시스템 자신)
		&UGameStatusSubsystem::DecreaseOxygenTick,   // 실행할 함수의 메모리 주소(포인터)
		1.0f,                                        // 실행 주기 (초)
		true                                         // 반복 여부 (true)
	);
}

void UGameStatusSubsystem::Deinitialize()
{
	// 메모리 누수 방지를 위해 타이머 해제
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(oxygenTimerHandle);
	}
	
	Super::Deinitialize();
}

void UGameStatusSubsystem::NotifyPlayerDeath()
{
	bIsPlayerDead = true;
	
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(oxygenTimerHandle);
	}
	
	// 구독자가 있다면 사망 소식을 방송합니다!
	if (OnPlayerDied.IsBound())
	{
		OnPlayerDied.Broadcast();
	}
}

void UGameStatusSubsystem::AddOxygen(float Amount)
{
	currentOxygen += Amount;
	// FMath::Clamp()는 언리얼 엔진(Unreal Engine) C++에서 제공하는 함수로, 특정 값을 지정한 최솟값과 최댓값 범위 내로 제한(고정)할 때 사용합니다.
	currentOxygen = FMath::Clamp(currentOxygen, 0.0f, 100.0f); 
}

void UGameStatusSubsystem::ConsumeOxygen(float Amount)
{
	if (bIsPlayerDead) return; // 이미 죽었으면 산소 감소 스톱!

	currentOxygen -= Amount;

	// 산소가 0 이하로 떨어졌을 때 = 사망 처리!
	if (currentOxygen <= 0.0f)
	{
		currentOxygen = 0.0f;
	}
}	

void UGameStatusSubsystem::DecreaseOxygenTick()
{
	if (bIsPlayerDead) return;
	
	// 1. [난이도 스케일링] 레벨 1당 초당 산소 소모량이 0.2씩 증가합니다 (1.0 -> 1.2 -> 1.4 ...)
	float ConsumptionRate = 1.0f + ((currentLevel - 1) * 0.2f);
	ConsumeOxygen(ConsumptionRate);
	
	// 2. 산소가 0 이하가 되면 처형 지시
	if (currentOxygen <= 0.0f)
	{
		ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		if (ADrillerCharacter* Driller = Cast<ADrillerCharacter>(PlayerChar))
		{
			// 캐릭터가 질식사 애니메이션을 틀고, 최종적으로 NotifyPlayerDeath()를 호출하여 타이머를 안전하게 끕니다.
			Driller->HandleDeath(true); 
		}
	}
}

void UGameStatusSubsystem::ProcessBlockDestructionScore(int32 BlockCount, bool bIsChainMatch, USoundBase* PopSound)
{
	int32 EarnedScore = 0;

	if (bIsChainMatch)
	{
		// 1. 낙하 연쇄 콤보 점수 공식
		EarnedScore = (BlockCount * 20) + ((BlockCount - 3) * 30);
	}
	else
	{
		// 2. 플레이어 직접 채굴 점수 공식
		// 1~3개 캘 때는 1개당 10점
		if (BlockCount < 4) 
		{
			EarnedScore = BlockCount * 10;
		}
		else // 4개 이상(콤보) 캘 때는 점수 폭발! (예: 4개면 40점이 아니라 80점, 5개면 150점...)
		{
			EarnedScore = (BlockCount * 10) + ((BlockCount - 3) * 20); 
		}
	}

	// 내부 점수 추가 및 고막 보호 사운드 재생 호출
	AddScore(EarnedScore);
	PlayBlockPopSFX(PopSound, BlockCount);
}

void UGameStatusSubsystem::PlayBGM(USoundBase* NewBGM, float FadeInTime)
{
	if (!NewBGM) return;

	// 이미 똑같은 배경음악이 재생 중이라면 중복 재생 방지
	if (CurrentBGMComponent && CurrentBGMComponent->Sound == NewBGM) return;

	// 기존에 재생 중이던 BGM이 있다면 페이드아웃하며 정지
	StopBGM(FadeInTime);

	// 2D 공간(화면 전체)에 사운드를 스폰합니다. (마지막 인자 true = 자동 파괴 켬)
	CurrentBGMComponent = UGameplayStatics::SpawnSound2D(GetWorld(), NewBGM, 1.0f, 1.0f, 0.0f, nullptr, true, true);
	
	if (CurrentBGMComponent)
	{
		// 부드럽게 볼륨을 올리며 시작합니다.
		CurrentBGMComponent->FadeIn(FadeInTime, 1.0f);
	}
}

void UGameStatusSubsystem::StopBGM(float FadeOutTime)
{
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
	{
		// 부드럽게 볼륨을 줄이며 끄고, 컴포넌트는 자동으로 소멸하게 둡니다.
		CurrentBGMComponent->FadeOut(FadeOutTime, 0.0f);
		CurrentBGMComponent = nullptr;
	}
}

void UGameStatusSubsystem::PlayBlockPopSFX(USoundBase* SFX, int32 ComboCount)
{
	if (!SFX) return;

	// 🎯 [도파민 폭발 공식] 터진 블록 개수(ComboCount)가 많아질수록 
	// 음높이(Pitch)를 5%씩 올려서 청각적 재미를 극대화합니다! (최대 2배 음높이 제한)
	float PitchMultiplier = 1.0f + (FMath::Max(0, ComboCount - 1) * 0.05f);
	PitchMultiplier = FMath::Clamp(PitchMultiplier, 1.0f, 2.0f);

	// 월드에 효과음을 탕! 쏴줍니다.
	UGameplayStatics::PlaySound2D(GetWorld(), SFX, 1.0f, PitchMultiplier);
}

void UGameStatusSubsystem::LoadGameData()
{
	// 디스크에 "PlayerSaveSlot"이라는 세이브 파일이 있는지 검사
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		// 있다면 불러와서 우리 클래스로 형변환(Cast)하여 캐싱합니다.
		USaveGame* LoadedData = UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0);
		currentSaveData = Cast<UMiniDrillerSaveGame>(LoadedData);
		
		// 🛠️ [안전장치] 구버전 세이브 파일 오염으로 캐스팅 실패 시, 강제로 새로 생성하여 치료합니다.
		if (currentSaveData == nullptr)
		{
			USaveGame* NewData = UGameplayStatics::CreateSaveGameObject(UMiniDrillerSaveGame::StaticClass());
			currentSaveData = Cast<UMiniDrillerSaveGame>(NewData);
		}
	}
	else
	{
		// 없다면 (처음 게임을 켰다면) 빈 그릇을 새로 하나 만듭니다.
		USaveGame* NewData = UGameplayStatics::CreateSaveGameObject(UMiniDrillerSaveGame::StaticClass());
		currentSaveData = Cast<UMiniDrillerSaveGame>(NewData);
	}
}

void UGameStatusSubsystem::SaveGameData()
{
	// 🛠️ [자가 치유 로직] 장부가 없으면 에러만 뿜고 끝나는 게 아니라, 그 자리에서 즉시 다시 불러옵니다!
	if (currentSaveData == nullptr)
	{
		LoadGameData(); // 긴급 복구!
		
		if (currentSaveData == nullptr)
		{
			return; // 그래도 안 되면 진짜 포기
		}
	}

	// 🛠️ [신기록 체크] 장부를 갱신하기 전에 현재 판의 기록이 기존 기록을 넘었는지 검사!
	bIsNewRecord = false;
	if (currentDepth > currentSaveData->maxDepth || currentScore > currentSaveData->maxScore || currentLevel > currentSaveData->maxLevel)
	{
		bIsNewRecord = true;
	}

	// 1. 기존 최고 기록과 방금 끝난 게임의 기록을 비교해서 더 큰 값(Max)으로 덮어씁니다.
	currentSaveData->maxDepth = FMath::Max(currentSaveData->maxDepth, currentDepth);
	currentSaveData->maxScore = FMath::Max(currentSaveData->maxScore, currentScore);
	currentSaveData->maxLevel = FMath::Max(currentSaveData->maxLevel, currentLevel);

	// 2. [업적 로직] 최고 깊이 기준으로 500m 당 뱃지 1개씩 계산! (소수점 버림)
	currentSaveData->unlockedBadgeCount = currentSaveData->maxDepth / 500;
	
	UGameplayStatics::SaveGameToSlot(currentSaveData, SaveSlotName, 0);
}

