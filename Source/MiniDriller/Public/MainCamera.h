// 플레이어의 좌우(X) 이동은 무시하고, 상하(Z) 깊이만 부드럽게 추적하는 전용 카메라 액터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MainCamera.generated.h"

UCLASS()
class MINIDRILLER_API AMainCamera : public AActor
{
	GENERATED_BODY()
	
public:	
	AMainCamera();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCameraComponent* cameraComponent;

	// 카메라가 쫓아갈 대상 (플레이어)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tracking")
	AActor* targetActor;

	// 플레이어를 화면 정중앙이 아닌 약간 아래쪽에 두기 위한 오프셋 (미스터 드릴러 특유의 시야 확보용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tracking")
	float zOffset = 0.0f;
};