// 사용자 입력 처리 및 드릴 채굴 로직을 담당하는 플레이어 캐릭터 클래스

#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "DrillerCharacter.generated.h"

// 전방 선언: 헤더 포함을 최소화하여 컴파일 속도를 높입니다.
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS(Blueprintable) // 외부 플러그인 때문에 한번 더 블루프린트 사용한다고 명시함
class MINIDRILLER_API ADrillerCharacter : public APaperZDCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ADrillerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void Tick( float DeltaTime ) override;

	// 입력 관련
#pragma region Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* defaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* moveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* digAction;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* playerInputComponent) override;
#pragma endregion
	
	// 플레이어 상태 관련 bool 변수
#pragma region State
	// 플레이어가 디깅중인지 상태 플래그
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDigging = false;
	
	// 옆으로 파고 있는지 확인용 상태 플래그
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDiggingSide = false;
	
	// 플레이어 사망(찌부러짐) 상태
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;
	
	// 질식사 상태 플래그
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsSuffocating = false;
	
	// 플레이어 승천(유령) 상태
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsFloating = false;
	
	// 플레이어가 블록 1개 이상 떨어졌는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsLongFalling = false;
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool GetIsDigging() const { return bIsDigging; }
	
	UFUNCTION(BlueprintCallable, Category = "State")
	bool GetIsDiggingSide() const { return bIsDiggingSide; }
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool GetIsDead() const { return bIsDead; }
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool GetIsSuffocating() const { return bIsSuffocating; }

	UFUNCTION(BlueprintPure, Category = "State")
	bool GetIsFloating() const { return bIsFloating; }
	
	UFUNCTION(BlueprintPure, Category = "State")
	bool GetIsLongFalling() const { return bIsLongFalling; }
#pragma endregion
	
	// 플레이어 이동 관련
#pragma region Movement
private:
	float currentFacingDir = 1.0f;     // 현재 바라보는 방향
	float climbEnableTimer = 0.0f; // 등판 허용까지 남은 시간
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (AllowPrivateAccess = "true"))
	float climbDelay = 0.3f; // 0.3초 정도 버티면 올라감
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (AllowPrivateAccess = "true"))
	float defaultStepHeight = 45.0f; // 블록 1칸(42)을 넘을 수 있는 높이
protected:
	// 배리어 블록 파괴 시 등 키 입력을 막기 위한 플래그
	UPROPERTY(BlueprintReadOnly, Category = "Move")
	bool bIsInputLocked = false;
public:
	void Move(const FInputActionValue& value); // 이동 관련 함수(좌,우, 블럭 1칸높이(벽) 이동)
#pragma endregion
	
	// 드릴 로직
#pragma region Drill
private:
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (AllowPrivateAccess = "true"))
	float rayIntersectionLength = 25.0f; // // 레이 사거리
public:
	void Dig(); // 채굴 명령
	UFUNCTION(BlueprintCallable, Category = "Dig")
	void StartDigging(); // 플레이어의 상태를 채굴중으로 바꾸고 속도 초기화
	UFUNCTION(BlueprintCallable, Category = "Dig")
	void StopDigging() { bIsDigging = false; } // 채굴 완료 알람 ABP에서 채굴 플립북 종료 시 호출
	// 외부에서 입력을 막거나 풀 때 사용하는 함수
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputLocked(bool bLock) { bIsInputLocked = bLock; }
#pragma endregion
	
	// 플레이어 사망 로직
#pragma region Death
public:
	// 애니메이션 대기를 위한 타이머
	struct FTimerHandle deathTimerHandle;
	void HandleDeath(bool bBySuffocation = false); // 산소 고갈 시 사망 연출 처리
	// ABP의 애님 노티파이(Anim Notify)가 Die1 종료 시점에 호출해 줄 함수
	UFUNCTION(BlueprintCallable, Category = "Death")
	void StartGhostFloat();
	void FinalizeDeath();   // 승천 2초 후 최종 게임오버 알림용
#pragma endregion

private:
	float lowestZ; // 플레이어가 도달한 최저점(가장 아래 위치)을 기억할 변수
	float fallStartZ = 0.0f; // 캐릭터가 하강을 시작한 Z 좌표 기록용
	const float blockSize = 42.0f; // 블록 1칸의 Z축 높이
	bool bIsFirstLanded = false;
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (AllowPrivateAccess = "true"))
	int32 depthPerBlock = 5; // 블록 1칸 파괴 시 증가할 깊이(m)
	UPROPERTY()
	class UGameStatusSubsystem* gameStatusSubsystem; // 전역 상태를 관리하는 서브시스템 캐싱용 포인터
	
public:
	// 맵 전환(배리어 파괴 방사형 폭발) 연출이 시작될 때 호출될 함수
	UFUNCTION(BlueprintCallable)
	void OnMapTransitionStart();
	// 맵 전환(배리어 파괴 후)이 완료되었을 때 호출될 함수
	UFUNCTION(BlueprintCallable)
	void OnMapTransitionComplete();
	// 레벨업(맵 전환 후 낙하) 시 재생할 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class USoundBase* levelUpSound;
	// 압사 당할 때 재생할 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class USoundBase* deadSound;
};
