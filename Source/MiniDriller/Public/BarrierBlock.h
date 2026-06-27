// 레벨업(100m) 구간에서 맵 전체를 가로막으며, 파괴 시 화면 내 모든 일반 블록을 제거하는 배리어 블록 클래스

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BarrierBlock.generated.h"

UCLASS()
class MINIDRILLER_API ABarrierBlock : public ABlock
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABarrierBlock();
	
	// 플레이어가 채굴을 시도했을 때의 특수 동작을 위해 오버라이드
	virtual void OnInteracted(class ADrillerCharacter* instigator) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void ForcePop(); // 맵 매니저에서 호출할 시각적 파괴 효과
};
