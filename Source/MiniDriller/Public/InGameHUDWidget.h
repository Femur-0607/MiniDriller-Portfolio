// 유지보수와 퍼포먼스를 극대화한 하이브리드 인게임 HUD 위젯 베이스 클래스

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "InGameHUDWidget.generated.h"


UCLASS()
class MINIDRILLER_API UInGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
	// --- 1. 코어 라이프사이클 ---
#pragma region CoreLifecycle
protected:
    // 유니티의 Start()와 같은 역할, 초기화 및 캐싱을 진행합니다.
    virtual void NativeConstruct() override;

    // 유니티의 Update()와 같은 역할, 매 프레임 UI 데이터를 동기화합니다.
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
#pragma endregion

    // --- 2. 인게임 UI 컴포넌트 바인딩 ---
#pragma region BoundWidgets_InGame
protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UImage* OxygenImage;
    
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Txt_Oxygen;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UHorizontalBox* HB_Depth;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UHorizontalBox* HB_Score;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UHorizontalBox* HB_Level;
    
    // --- UI 애니메이션 바인딩 ---
    // 주의: Transient는 이 변수가 디스크에 저장될 필요 없는 휘발성 메모리임을 엔진에 알립니다.
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    class UWidgetAnimation* Anim_GameStart;
    
    // 레벨업 시 재생될 애니메이션
    UPROPERTY(Transient, meta = (BindWidgetAnim))
    class UWidgetAnimation* Anim_LevelUp;
#pragma endregion

    // --- 3. 게임 오버 UI 컴포넌트 바인딩 ---
#pragma region BoundWidgets_GameOver
protected:
    // 게임 오버 UI들을 담고 있는 최상위 그룹 (Overlay, Border 등)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UWidget* GameOverGroup;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UButton* Btn_Restart;
    
    UPROPERTY(meta = (BindWidget))
    class UButton* Btn_Title;

    // --- 결과창 UI 바인딩 ---
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* Txt_ResultDepth;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* Txt_ResultScore;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* Txt_ResultLevel;

    // "최고 기록!" 스프라이트를 띄울 이미지
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UImage* Img_NewRecord;
#pragma endregion

    // --- 4. 에셋 및 리소스 (에디터 할당용) ---
#pragma region ResourcesAndAssets
public:
    // 에디터에서 0부터 9까지 순서대로 채워넣을 숫자 스프라이트 배열
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI | Resources")
    TArray<class UPaperSprite*> NumberSprites;
    
    // 산소 효과 원본 머티리얼을 직접 지정할 변수!
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI | Resources")
    class UMaterialInterface* OxygenBaseMaterial;
    
    UPROPERTY(EditDefaultsOnly, Category = "UI | Audio")
    class USoundBase* gameStartSound;

    UPROPERTY(EditDefaultsOnly, Category = "UI | Audio")
    class USoundBase* GameOverSound;
#pragma endregion

    // --- 5. 캐싱 및 내부 상태 데이터 ---
#pragma region InternalState
private:
    // 싱글톤 서브시스템 캐싱용 포인터
    class UGameStatusSubsystem* statusSubsystemRef;
    // 동적 제어할 산소 머티리얼 인스턴스
    class UMaterialInstanceDynamic* oxygenDynamicMaterial;
    
    // 값의 변화를 감지하기 위한 캐싱 변수들
    float lastOxygen = -1.0f;
    int32 lastDepth = -1;
    int32 lastScore = -1;
    int32 lastLevel = -1;
#pragma endregion

    // --- 6. 핵심 알고리즘 및 이벤트 콜백 ---
#pragma region CoreLogic
protected:
    // 서브시스템의 죽음 방송을 들을 콜백 함수
    UFUNCTION()
    void HandlePlayerDeath();
    
    // 다시 시작 버튼이 눌렸을 때 실행될 함수
    UFUNCTION()
    void OnRestartClicked();
    
    // 타이틀 버튼이 눌렸을 때 실행될 함수
    UFUNCTION()
    void OnTitleClicked(); 

private:
    // 숫자를 자릿수별로 쪼개서 수평 박스의 이미지들을 교체하는 마법의 함수
    void UpdateNumberUI(int32 Value, class UHorizontalBox* TargetBox);

    // 산소 퍼센트를 머티리얼 파라미터로 넘겨주는 함수
    void UpdateOxygenUI(float CurrentOxygen);
#pragma endregion
};