// 맵(월드)의 전반적인 상태(점수, 산소, 게임오버 등)를 관리하는 서브시스템
#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameStatusSubsystem.generated.h"

// 블루프린트에서도 쓸 수 있는 다이내믹 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDiedDelegate);

UCLASS(BlueprintType)
class MINIDRILLER_API UGameStatusSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
	// --- 1. 코어 초기화 시스템 ---
#pragma region Core Initialization
protected:
	// 서브시스템이 생성될 때 엔진이 호출해주는 초기화 함수
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
	// 서브시스템이 파괴될 때 호출되는 함수 (타이머 정리용)
	virtual void Deinitialize() override;
#pragma endregion
	// --- 2. 전역 상태 및 이벤트 ---
#pragma region Global State & Events
public:
	// 다른 곳(UI 등)에서 구독할 이벤트 변수
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerDiedDelegate OnPlayerDied;

	// 플레이어 사망 감지
	UPROPERTY(BlueprintReadOnly, Category = "Game Data")
	bool bIsPlayerDead = false;

	// 캐릭터가 죽었을 때 호출할 함수 (델리게이트 방송 및 타이머 정지)
	UFUNCTION(BlueprintCallable, Category = "Game Logic")
	void NotifyPlayerDeath();

	// 현재 게임 오버 상태인지 확인하는 함수
	// 산소가 0 이하로 떨어졌거나(OR) 플레이어가 사망했다면 게임 오버!
	UFUNCTION(BlueprintPure, Category = "Game Logic")
	bool IsGameOver() const { return (currentOxygen <= 0.f) || bIsPlayerDead; }
#pragma endregion
	// --- 3. 산소 (생명) 시스템 ---
#pragma region Oxygen System
public:
    UPROPERTY(BlueprintReadWrite, Category = "Game Data")
    float currentOxygen = 100.0f; // 초기 산소량
    
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void AddOxygen(float Amount);
    
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void ConsumeOxygen(float Amount);
    
private:
    UPROPERTY()
    FTimerHandle oxygenTimerHandle;

    UFUNCTION()
    void DecreaseOxygenTick();
#pragma endregion
    // --- 4. 깊이(Depth) 및 레벨 시스템 ---
#pragma region Depth System
public:
    UPROPERTY(BlueprintReadWrite, Category = "Game Data")
    int32 currentDepth = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Game Data")
    int32 currentLevel = 1;
    
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void AddDepth(int32 Amount) { currentDepth += Amount; }
    
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void LevelUp() { currentLevel++; }
#pragma endregion
    // --- 5. 점수(Score) 관리 시스템 ---
#pragma region Score System
public:
    UPROPERTY(BlueprintReadWrite, Category = "Game Data")
    int32 currentScore = 0;

    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void AddScore(int32 Amount) { currentScore += Amount; }
	// 직접 채굴인지 낙하 연쇄인지 확인 후 점수 및 사운드 재생
    UFUNCTION()
    void ProcessBlockDestructionScore(int32 BlockCount, bool bIsChainMatch, USoundBase* PopSound);
#pragma endregion
    // --- 6. 사운드(오디오) 매니저 ---
#pragma region Audio System
protected:
    // 현재 재생 중인 BGM을 제어하기 위한 오디오 컴포넌트 포인터
    UPROPERTY()
    class UAudioComponent* CurrentBGMComponent;

public:
    // 새로운 배경음악을 부드럽게 페이드인하며 재생
    UFUNCTION(BlueprintCallable, Category = "Sound Manager")
    void PlayBGM(class USoundBase* NewBGM, float FadeInTime = 1.0f);

    // 현재 배경음악을 부드럽게 페이드아웃하며 정지
    UFUNCTION(BlueprintCallable, Category = "Sound Manager")
    void StopBGM(float FadeOutTime = 1.0f);

    // 블록 파괴 전용 효과음 (콤보 수에 따라 피치 조절 기능 내장)
    UFUNCTION(BlueprintCallable, Category = "Sound Manager")
    void PlayBlockPopSFX(class USoundBase* SFX, int32 ComboCount);
#pragma endregion
    // --- 7. 세이브 & 로드 시스템 ---
#pragma region Save System
public:
    // 세이브 파일을 저장하고 불러올 때 쓸 슬롯(파일) 이름
    const FString SaveSlotName = TEXT("PlayerSaveSlot");

    // 메모리에 들고 있을 세이브 데이터 원본 포인터
    UPROPERTY(BlueprintReadOnly, Category = "Save Data")
    class UMiniDrillerSaveGame* currentSaveData;
    
    UPROPERTY(BlueprintReadOnly, Category = "Game Data")
    bool bIsNewRecord = false;

    // 게임 켤 때(초기화) 데이터 불러오기
    UFUNCTION(BlueprintCallable, Category = "Save Data")
    void LoadGameData();

    // 게임 오버 시 기록 갱신 및 저장하기
    UFUNCTION(BlueprintCallable, Category = "Save Data")
    void SaveGameData();
#pragma endregion
};