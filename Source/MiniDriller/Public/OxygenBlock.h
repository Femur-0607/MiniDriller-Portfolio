// 상호작용 시 플레이어의 산소를 회복시켜주는 특수 아이템 블록

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "OxygenBlock.generated.h"

UCLASS()
class MINIDRILLER_API AOxygenBlock : public ABlock
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AOxygenBlock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	// 다형성의 핵심: 부모(ABlock)의 가상 함수를 덮어쓰기(Override) 합니다.
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void OnInteracted(class ADrillerCharacter* Player) override;
	virtual void ResetBlockState() override;
	UPROPERTY(EditDefaultsOnly)
	class USoundBase* HealSound;
};
