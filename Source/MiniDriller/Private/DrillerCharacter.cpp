// Fill out your copyright notice in the Description page of Project Settings.


#include "DrillerCharacter.h"
#include "Block.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PaperFlipbookComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameStatusSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ADrillerCharacter::ADrillerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 캐릭터 자체가 컨트롤러나 이동 방향에 따라 도는 것을 방지
	bUseControllerRotationYaw = false;
	
	if (GetCharacterMovement())
	{
		// PaperZD가 애니메이션 상태(Velocity)를 기반으로 작동하게 하되, 캐릭터 자체가 회전하는 것은 막습니다.
		GetCharacterMovement()->bOrientRotationToMovement = false;
        
		// Y축 이동 고정
		GetCharacterMovement()->bConstrainToPlane = true;
		GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, 1.0f, 0.0f));
		
		// 시작할 때는 계단을 못 올라가게 0으로 설정합니다. (기본값 45)
		GetCharacterMovement()->MaxStepHeight = 0.0f;
	}
}

// Called when the game starts or when spawned
void ADrillerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 1. Mapping Context 등록
	if (APlayerController* pc = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
		{
			subsystem->AddMappingContext(defaultMappingContext, 0);
		}
	}
	
	// 시작할 때 딱 한 번만 서브시스템을 찾아서 캐싱해 둡니다.
	gameStatusSubsystem = GetWorld()->GetSubsystem<UGameStatusSubsystem>();
}

void ADrillerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 승천(Die2) 상태라면 매 프레임 위로(Z축 양수) 이동시킵니다.
	if (bIsFloating)
	{
		// 스프라이트만 위로 부드럽게 띄웁니다.
		// AddRelativeLocation는 객체나 컴포넌트를 부모(Parent) 트랜스폼의 좌표계를 기준으로 지정한 벡터(Vector) 값만큼 상대적으로 이동시키는 함수
		GetSprite()->AddRelativeLocation(FVector(0.f, 0.f, 150.f * DeltaTime));
	}
	
	// 현재 Z축 위치를 가져옵니다.
	float currentZ = GetActorLocation().Z;

	// 🌟 [추가] 공중에서 떨어지다가 '처음으로' 땅에 닿았을 때 기준점을 세팅합니다.
	if (!bIsFirstLanded && GetCharacterMovement()->MovementMode != MOVE_Falling)
	{
		bIsFirstLanded = true;
		lowestZ = currentZ; // 이 순간의 Z 좌표가 내 진짜 시작점!
	}

	// 🌟 [수정] 처음 땅에 닿은 '이후'에만 깊이를 측정합니다.
	if (bIsFirstLanded)
	{
		// 현재 위치가 '기록된 최저점'보다 (블록 1칸 blockSize) 이상 더 아래로 내려갔다면?
		if (currentZ <= lowestZ - blockSize)
		{
			// 최저점을 blockSize 아래로 갱신합니다.
			lowestZ -= blockSize; 

			// 서브시스템에 한칸 내려갔다고 전달
			if (gameStatusSubsystem)
			{
				gameStatusSubsystem->AddDepth(depthPerBlock); 
			}
		}
	}
}

// Called to bind functionality to input
void ADrillerCharacter::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);
	
	// Action 바인딩
	if (UEnhancedInputComponent* ei = CastChecked<UEnhancedInputComponent>(playerInputComponent))
	{
		ei->BindAction(moveAction, ETriggerEvent::Triggered, this, &ADrillerCharacter::Move);
		ei->BindAction(digAction, ETriggerEvent::Started, this, &ADrillerCharacter::Dig);
	}
}

void ADrillerCharacter::Move(const FInputActionValue& value)
{
	if (bIsDead || bIsDigging || bIsInputLocked) return;
    
	float moveVector = value.Get<float>();
	if (moveVector == 0.f) return;
	
	// FMath::Sign()입력된 숫자가 양수인지, 음수인지, 0인지만 판단해서 딱 떨어지는 정수형 실수로 반환해 주는 아주 유용한 '부호 판독기'
	float inputDir = FMath::Sign(moveVector);
    
	// 1. 방향 전환 시 즉시 회전
	if (inputDir != currentFacingDir)
	{
		currentFacingDir = inputDir;
		// SetRelativeRotation은 부모 컴포넌트(또는 루트)를 기준으로 자식 컴포넌트의 상대적인 회전값(Local Rotation)을 설정하는 언리얼 엔진의 함수
		GetSprite()->SetRelativeRotation(FRotator(0.f, (currentFacingDir < 0.f) ? 0.f : 180.f, 0.f));
		climbEnableTimer = 0.0f; // 방향 바꾸면 타이머 초기화
	}
	
	//DrillerCharacter.cpp -> Move()
	// 2. 앞에 벽(블록)이 있는지 체크
	FHitResult wallHit;
	FVector start = GetActorLocation();
	FVector end = start + FVector(inputDir * rayIntersectionLength, 0.0f, 0.0f);
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);
	// LineTraceSingleByChannel()은 지정한 콜리전 채널을 기준으로 가상의 광선(Ray)을 쏴서, 경로상에 가장 먼저 부딪히는(Block) 물체 하나를 감지하는 C++ 함수
	bool bHitBlock = GetWorld()->LineTraceSingleByChannel
	(wallHit, start, end, ECC_Visibility, params) && Cast<ABlock>(wallHit.GetActor());

	if (bHitBlock)
	{
		// 벽을 향해 계속 밀고 있다면 타이머 증가
		// Move가 매 프레임 호출되므로 누적 가능
		climbEnableTimer += GetWorld()->GetDeltaSeconds();

		if (climbEnableTimer >= climbDelay)
		{
			// 지연 시간이 지났으므로 계단 오르기 능력 개방!
			GetCharacterMovement()->MaxStepHeight = defaultStepHeight;
		}
	}
	else
	{
		// 벽이 없으면 즉시 능력 회수 및 타이머 초기화
		climbEnableTimer = 0.0f;
		GetCharacterMovement()->MaxStepHeight = 0.0f;
	}

	// 이동 입력
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), moveVector);
}

void ADrillerCharacter::Dig()
{
	if (bIsDead || bIsDigging || bIsInputLocked) return;
	
	StartDigging();
	
	// 1. 레이캐스트 시작점(캐릭터의 현재 위치)을 가져옵니다.
	FVector rayStart = GetActorLocation();
	// 2. 기본 채굴 방향은 아래쪽(Z: -1)으로 설정합니다.
	FVector digDirection = FVector(0.f, 0.f, -1.f);
	// 2-1. 캐릭터에 입력된 마지막 이동 벡터를 가져옵니다.
	FVector lastInput = GetLastMovementInputVector();
	
	// 2-2. X축 입력이 존재한다면 (좌/우 이동 키를 누르고 있다면) 방향을 덮어씁니다.
	if (!FMath::IsNearlyZero(lastInput.X)) // FMath::IsNearlyZero()는 특정 부동소수점 값이 사실상 0에 가까운지 확인하는 함수
	{
		// 입력값의 부호(+1 혹은 -1)에 따라 파는 방향을 결정합니다.
		float dirX = FMath::Sign(lastInput.X);
		digDirection = FVector(dirX, 0.f, 0.f);
		bIsDiggingSide = true; // 옆으로 파기 상태 ON
	}
	else
	{
		bIsDiggingSide = false; // 아래로 파기 상태 (X축 입력이 없을 때)
	}
	
	// 3. 레이캐스트 끝점(시작점 + (바라보는 방향 * 채굴 사거리))을 계산합니다.
	FVector rayEnd = rayStart + (digDirection * rayIntersectionLength);
	// 4. FHitResult 구조체를 선언하여 충돌 결과를 담을 바구니를 준비합니다.
	FHitResult hit;
	// 5. GetWorld()->LineTraceSingleByChannel(...) 함수를 호출하여 레이를 발사합니다.
	FCollisionQueryParams params; // FCollisionQueryParams는 언리얼 엔진에서 레이캐스팅(Raycast), 스윕(Sweep), 오버랩(Overlap) 등의 콜리전 쿼리(충돌 검사)를 수행할 때 충돌 탐지 조건과 매개변수를 정의하는 C++ 구조체입니다.
	params.AddIgnoredActor(this); // 자기 자신을 레이캐스트 대상에서 제외합니다.
	
	// 에디터 환경에서만 실행되는 시각적 디버그 라인 로직
#if WITH_EDITOR
	// 빨간색(FColor::Red) 선을 1.0초(지속시간) 동안, 두께 2.0f로 그립니다.
	DrawDebugLine(GetWorld(), rayStart, rayEnd, FColor::Red, false, 1.0f, 0, 2.0f);
#endif
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(hit, rayStart, rayEnd, ECC_Visibility, params);
	
	// 6. 충돌한 액터가 있다면 HitResult에서 가져와 ABlock 클래스로 Cast<ABlock>을 시도합니다.
	if (bHit)
	{
		// 부딪힌 액터가 ABlock 타입인지 확인하고 안전하게 캐스팅합니다.
		ABlock* HitBlock = Cast<ABlock>(hit.GetActor());
    
		// 7. 캐스팅에 성공했다면 해당 블록의 OnInteracted(this) 함수를 호출합니다.
		if (HitBlock != nullptr)
		{
			HitBlock->OnInteracted(this);
			
			// 아래로 파기에 성공했다면 속도 주입 (스매시)
			if (!bIsDiggingSide)
			{
				// ✅ [핵심 해결책] 엔진에게 강제로 "낙하 모드"로 전환하라고 명령합니다!
				// 이렇게 하면 42만큼 얕게 파더라도 계단 스냅(순간이동)이 발생하지 않고 부드럽게 떨어집니다.
				GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			}
		}
	}
}

void ADrillerCharacter::StartDigging()
{
	if (!bIsDigging)
	{
		bIsDigging = true;
		
		// 현재 속도에서 좌우(X)만 0으로 만들고, 중력(Z)은 그대로 둡니다!
		FVector currentVelocity = GetCharacterMovement()->Velocity;
		currentVelocity.X = 0.0f; 
		GetCharacterMovement()->Velocity = currentVelocity;
	}
}

void ADrillerCharacter::HandleDeath(bool bBySuffocation)
{
	if (bIsDead) return;
	// 일단 죽음 처리 (이동, 입력 통제용 공통 플래그)
	bIsDead = true; 
	// 사인(원인) 기록 (PaperZD 애니메이션 분기용)
	bIsSuffocating = bBySuffocation;
	
	// 사망 사운드 재생
	if (deadSound)
	{
		UGameplayStatics::PlaySound2D(this, deadSound);
	}

	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADrillerCharacter::StartGhostFloat()
{
	// 승천 상태 ON! (Tick 함수에서 위로 올라가기 시작하고, ABP가 'Die2' 애니메이션을 틉니다)
	bIsFloating = true;

	// 2.0f 뒤에 애니메이션을 끝내고 이벤트를 호출합니다.
	GetWorld()->GetTimerManager().SetTimer(deathTimerHandle, this, &ADrillerCharacter::FinalizeDeath, 2.0f, false);
}

void ADrillerCharacter::FinalizeDeath()
{
	// 승천 종료
	bIsFloating = false;
    
	// 시각적으로 완전히 투명하게(사라지게) 만듭니다.
	GetSprite()->SetHiddenInGame(true);

	// 서브시스템에 사망 소식을 방송(Broadcast)하여 게임 오버 창을 띄우게 합니다!
	if (gameStatusSubsystem)
	{
		gameStatusSubsystem->NotifyPlayerDeath(); 
	}
}

void ADrillerCharacter::OnMapTransitionStart()
{
	// 1. 중력 무시 및 떨어지던 가속도 초기화 (타임 스탑 연출)
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GravityScale = 0.0f; 
		Movement->Velocity = FVector::ZeroVector; 
	}

	// 2. 연출 도중 플레이어가 조작하지 못하도록 입력 잠금!
	bIsInputLocked = true; 
}

void ADrillerCharacter::OnMapTransitionComplete()
{
	// 1. 중력 복구
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GravityScale = 1.0f; 
	}
	
	// 2. 입력 잠금 해제
	bIsInputLocked = false;
	
	// 3. 중력이 복구되어 떨어지는 순간 레벨업 사운드 재생!
	if (levelUpSound)
	{
		UGameplayStatics::PlaySound2D(this, levelUpSound);
	}
}