// 로컬 디스크에 플레이어의 최고 기록과 업적 데이터를 직렬화하여 저장하는 데이터 컨테이너 클래스

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MiniDrillerSaveGame.generated.h"

UCLASS()
class MINIDRILLER_API UMiniDrillerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 최고 도달 깊이
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 maxDepth = 0;

	// 최고 점수
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 maxScore = 0;

	// 최고 도달 레벨
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 maxLevel = 0;

	// [업적] 500m 단위로 해금된 뱃지 개수
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Achievement")
	int32 unlockedBadgeCount = 0;
};