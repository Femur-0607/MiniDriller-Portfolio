// 플레이어의 좌우(X) 이동은 무시하고, 상하(Z) 깊이만 부드럽게 추적하는 전용 카메라 액터

#include "MainCamera.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

AMainCamera::AMainCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 카메라 컴포넌트 생성 및 루트로 설정
	cameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	RootComponent = cameraComponent;
}

void AMainCamera::BeginPlay()
{
	Super::BeginPlay();

	// 1. 월드에 있는 플레이어 캐릭터를 찾아 타겟으로 자동 지정합니다.
	targetActor = UGameplayStatics::GetPlayerCharacter(this, 0);

	// 2. 게임이 시작되면 나 자신(이 카메라)을 플레이어의 주 시점(ViewTarget)으로 강제 설정합니다.
	APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0);
	if (pc)
	{
		pc->SetViewTarget(this);
	}
}

void AMainCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (targetActor)
	{
		// 현재 카메라의 위치
		FVector currentPos = GetActorLocation();
		
		// 타겟(플레이어)의 Z축 목표 위치
		float targetZ = targetActor->GetActorLocation().Z + zOffset;

		// [핵심 로직] X, Y축은 지금 위치(고정)를 유지하고, Z축만 플레이어를 향해 부드럽게 보간(Interp)합니다.
		// 5.0f는 추적 속도입니다. 이 수치를 조절해 카메라의 묵직함을 튜닝할 수 있습니다.
		currentPos.Z = FMath::FInterpTo(currentPos.Z, targetZ, DeltaTime, 5.0f);

		SetActorLocation(currentPos);
	}
}