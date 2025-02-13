# [8번 과제] 게임 루프 및 UI 재설계하기
---
## 2️⃣ 과제 진행 개요 ⚔️

### 1단계 : 멀티 웨이브 구성하기
우선 각 레벨별로 웨이브가 3단계로 이루어지게 구성했습니다.
구현하기 위해 우선 기존에 레벨단위에서 `FTimerHandle`를 웨이브단위로 바꿨기 때문에 변수명을 `WaveTimerHandle`를 `GameState.h`에 선언했고,
```cpp
	GetWorldTimerManager().SetTimer(
		WaveTimerHandle,
		this,
		&ASpartaGameState::OnWaveTimeUP,
		LevelDuration,
		false
	);
```
바꿔주었습니다. 
```cpp
void ASpartaGameState::StartLevel()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->ShowGameHUD();
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			CurrentLevelIndex = SpartaGameInstance->CurrentLevelIndex;
		}
	}

	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	int32 ItemToSpawn;
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);
	ItemToSpawn = 10 * CurrentWave;
	for (int32 i = 0; i < ItemToSpawn; i++)
	{
		if (FoundVolumes.Num() > 0)
		{
			ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
			if (SpawnVolume)
			{
				AActor* SpawnedActor = SpawnVolume->SpawnRandomItem();
				if (SpawnedActor && SpawnedActor->IsA(ACoinItem::StaticClass()))
				{
					SpawnedCoinCount++;
				}
			}
		}
	}
```
기존 로직을 그대로 사용하고 이제 웨이브마다 스폰되는 아이템 갯수의 차이를 줬습니다 웨이브 * 10으로 1웨이브에 10 2웨이브에 20개 3웨이브에 30개 아이템이 스폰되게 설정했습니다.

그 후 웨이브시간이 끝났을 때 호출되는 `OnWaveTimeUP()` 함수 로직을 아래와 같이 짜봤습니다.
```cpp
void ASpartaGameState::OnWaveTimeUP()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);ㅋ`

	TArray<UClass*> ItemClasses;
	if (ACoinItem::StaticClass()) ItemClasses.Add(ACoinItem::StaticClass());
	if (AHealingItem::StaticClass()) ItemClasses.Add(AHealingItem::StaticClass());
	if (AMineItem::StaticClass()) ItemClasses.Add(AMineItem::StaticClass());
	if (!GetWorld())
	{
		return;
	}

	for (UClass* ItemClass : ItemClasses)
	{
		if (!ItemClass) continue; // nullptr 방지

		TArray<AActor*> FoundItems;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ItemClass, FoundItems);

		for (AActor* Item : FoundItems)
		{
			if (Item)
			{
				Item->Destroy();
			}
		}
	}

	if (CurrentWave >= 3)
	{
		EndLevel();
	}
	else
	{
		CurrentWave++;
		StartLevel();
	}
}
```
웨이브시간이 끝나면 기존에 오버랩 되지 않은 즉, 사용되지 않은 아이템들이 그대로 남아있는 게임흐름과 맡지않은 부분이 있어서
```cpp
	TArray<UClass*> ItemClasses;
	if (ACoinItem::StaticClass()) ItemClasses.Add(ACoinItem::StaticClass());
	if (AHealingItem::StaticClass()) ItemClasses.Add(AHealingItem::StaticClass());
	if (AMineItem::StaticClass()) ItemClasses.Add(AMineItem::StaticClass());
	if (!GetWorld())
	{
		return;
	}

	for (UClass* ItemClass : ItemClasses)
	{
		if (!ItemClass) continue; // nullptr 방지

		TArray<AActor*> FoundItems;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ItemClass, FoundItems);

		for (AActor* Item : FoundItems)
		{
			if (Item)
			{
				Item->Destroy();
			}
		}
	}
```
웨이브 시간이 끝나고 새로운 웨이브로 가기전에 아이템들을 모두 삭제하는 로직을 구현했습니다.

![SpartaProject 프리뷰  NetMode_ Standalone 0  (64-bit_PC D3D SM6) 2025-02-13 15-09-15](https://github.com/user-attachments/assets/2854564b-a1d8-47d0-b8a4-f56dead5f110)
![SpartaProject 프리뷰  NetMode_ Standalone 0  (64-bit_PC D3D SM6) 2025-02-13 15-09-39](https://github.com/user-attachments/assets/2fabc1ce-6558-47c7-be26-8b2bbf5ae8d5)


### 2단계 : 현재 UI 분석 및 요구사항 정리
#### 기존의 HUD 와 Menu
![](https://velog.velcdn.com/images/xotepsin/post/6384437f-6bfb-4ebb-ac2c-7a9387de7b32/image.png)
![](https://velog.velcdn.com/images/xotepsin/post/33e8d2fe-c6a6-446b-a0ee-eee32c855fa4/image.png)
![](https://velog.velcdn.com/images/xotepsin/post/ef913815-7764-421c-99bb-ba627a802617/image.png)
![](https://velog.velcdn.com/images/xotepsin/post/7b27ea91-80ed-41fd-9362-05d8f7c4e5b0/image.png)

- 기존 메뉴와 HUD입니다 우선 HUD에서는 웨이브가 추가됨으로써 Level만 출력하는 것이 아닌 Wave도 출력해야하는 필요성을 느꼈습니다. 캐릭터 위에 텍스트로 체력을 표시하는 것 보다는, 막대기로 표시하는게 더 좋을 것 같다는 생각에 막대기로 바꿨고 이로 인해 막대기로 표현하다보니 유저의 체력이 몇인지 수치상으로 정확하게 보여주는게 좋을 것 같아 HUD에 표시하고, 현재 웨이브의 남은 코인수를 알 수 있도록 표시해주는 것 이 좋다고 생각했습니다.
- 다음으로는 Menu부분인데요 우선 Menu는 MainMenu에서 모두 처리하는게 아니라 Menu를 좀더 다양하게 GameOverMenu MainMenu 그리고 클리어했을 때 나오는 창인 ClearMenu를 추가하는것이 좋아보였습니다.
### 3단계 : HUD 및 Menu UI 재설계
![](https://velog.velcdn.com/images/xotepsin/post/9ec0d798-2633-4f67-b1c5-6cd6e7f8ad26/image.png)
2단계에서 말했듯 HUD와 Menu UI를 추가하고 재설계해봤습니다.
#### HUD
![](https://velog.velcdn.com/images/xotepsin/post/2443a735-6330-4998-8b1c-96124f9d36ed/image.png)
웨이브의 남은 코인수, 현재 플레이어의 체력을 추가하고, 현재레벨 뿐만이아닌 현재 웨이브도 표시하게 수정하였습니다.
```cpp
if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
{
    LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d / Wave: %d"), CurrentLevelIndex + 1, CurrentWave)));
}

if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("CollectedCoin"))))
{
    LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Coin Collected: %d/%d"), CollectedCoinCount, SpawnedCoinCount)));
}
if (ASpartaCharacter* SpartaCharacter = Cast<ASpartaCharacter>(PlayerController->GetCharacter()))
{
    if (UTextBlock* HealthText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Health"))))
    {
        HealthText->SetText(FText::FromString(FString::Printf(TEXT("Health: %.0f/%.0f"), SpartaCharacter->GetHealth(), SpartaCharacter->GetMaxHealth())));
    }
}
```
기존 `UpdateHUD()`함수에 추가하고 수정했습니다.
![](https://velog.velcdn.com/images/xotepsin/post/471fe783-7698-46ac-a09c-95a1eb26c0b4/image.png)

![SpartaProject 프리뷰  NetMode_ Standalone 0  (64-bit_PC D3D SM6) 2025-02-13 15-08-41](https://github.com/user-attachments/assets/21a2b003-f8b9-4e21-b412-d11892779f87)


```cpp
void ASpartaCharacter::UpdateOverHeadHP()
{
	if (!OverheadWidget) return;

	UUserWidget* OverheadWidgetInstance = OverheadWidget->GetUserWidgetObject();
	if (!OverheadWidgetInstance) return;

	if (UProgressBar* HPBar = Cast<UProgressBar>(OverheadWidgetInstance->GetWidgetFromName(TEXT("HP_Bar"))))
	{
		float HealthPercent = Health / MaxHealth; // 0.0 ~ 1.0 사이 값
		HPBar->SetPercent(HealthPercent);
	}
}
```
체력바를 구현하기 위해 기존 `UpdateOverHeadHP()`함수를 수정하고 기존에 구현하기 위해 `include` 했던`TextBox.h` 대신 `ProgressBar.h`를 `include`했습니다.

![SpartaProject 프리뷰  NetMode_ Standalone 0  (64-bit_PC D3D SM6) 2025-02-13 15-53-57](https://github.com/user-attachments/assets/c7cc7133-54df-4070-9d2e-3047bf700e7f)


#### MainMenu
![](https://velog.velcdn.com/images/xotepsin/post/787ad578-03ed-4a2f-8055-ecd84d5c657e/image.png)
메인메뉴는 단순하게 `Start`버튼과 `Exit`버튼 두개로 구현했습니다.
```cpp
void ASpartaPlayerController::ExitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
```
Exit는 `ExitGame()`함수를 구현하고 블루프린트에서 이벤트그래프로 마무리 해주었습니다.
![](https://velog.velcdn.com/images/xotepsin/post/f5c0bbb1-a0ca-40e3-a230-f7e43b49f709/image.png)

#### GameOverMenu
기존에는 MainMenu UI와 GameOverMenuUI를 같은 `WBP`에서 다뤘지만 저는 분리했습니다.
![](https://velog.velcdn.com/images/xotepsin/post/38e3fcc8-75ac-4028-90a2-57ace0f5246d/image.png)
기존에서 아래 `MainMenu`로 갈 수 있는 Main버튼만 달아주었습니다.
![](https://velog.velcdn.com/images/xotepsin/post/7cae5e9a-f833-44bb-a3e6-27c34b82af25/image.png)

![SpartaProject 프리뷰  NetMode_ Standalone 0  (64-bit_PC D3D SM6) 2025-02-13 15-08-25](https://github.com/user-attachments/assets/45a7bde8-8d9a-463a-9a03-d16cb9fd78f9)


#### Clear

![](https://velog.velcdn.com/images/xotepsin/post/7925337e-eba6-4b14-887d-edd19f8b491a/image.png)
기존에서는 체력이 없어서 플레이어가 죽은것과 모든 레벨의 코인을다먹거나 시간이 지나서 끝나는 경우도 GameOver가 되었습니다. 게임 흐름과 맞지 않다고 생각하여 Clear `WBP`를 하나 더 만들어서 다뤘습니다.

클리어를 하고나면 클리어까지 총 걸린시간, 총 점수, 수집한 코인의 개수를 나오게 구현했습니다.

아래는 `ShowClearMenu()` 코드입니다.
```cpp
void ASpartaPlayerController::ShowClearMenu()
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->RemoveFromParent();
		MainMenuWidgetInstance = nullptr;
	}
	if (GameOverWidgetInstance)
	{
		GameOverWidgetInstance->RemoveFromParent();
		GameOverWidgetInstance = nullptr;
	}
	if (ClearWidgetInstance)
	{
		ClearWidgetInstance->RemoveFromParent();
		ClearWidgetInstance = nullptr;
	}

	if (ClearWidgetClass)
	{
		ClearWidgetInstance = CreateWidget<UUserWidget>(this, ClearWidgetClass);
		if (ClearWidgetInstance)
		{
			ClearWidgetInstance->AddToViewport();
			bShowMouseCursor = true;
			SetInputMode(FInputModeUIOnly());
		}
		UFunction* PlayAnimFunc = ClearWidgetInstance->FindFunction(FName("PlayAnimClear"));
		if (PlayAnimFunc)
		{
			ClearWidgetInstance->ProcessEvent(PlayAnimFunc, nullptr);
		}
		if (UTextBlock* TotalScoreText = Cast<UTextBlock>(ClearWidgetInstance->GetWidgetFromName("TotalScoreText")))
		{
			if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(UGameplayStatics::GetGameInstance(this)))
			{
				TotalScoreText->SetText(FText::FromString(
					FString::Printf(TEXT("Total Score: %d"),
						SpartaGameInstance->TotalScore)
				));
			}
		}
		if (UTextBlock* TotalScoreText = Cast<UTextBlock>(ClearWidgetInstance->GetWidgetFromName("TotalTimeText")))
		{
			if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(UGameplayStatics::GetGameInstance(this)))
			{
				TotalScoreText->SetText(FText::FromString(
					FString::Printf(TEXT("Total Time: %.1f"),
						SpartaGameInstance->TotalTime)
				));
			}
		}
		if (UTextBlock* TotalScoreText = Cast<UTextBlock>(ClearWidgetInstance->GetWidgetFromName("CollectCoinText")))
		{
			if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(UGameplayStatics::GetGameInstance(this)))
			{
				TotalScoreText->SetText(FText::FromString(
					FString::Printf(TEXT("Collected Coin: %d"),
						SpartaGameInstance->CollectCoin)
				));
			}
		}
	}
}
```

수집한 코인의 수와 클리어까지 걸린시간을 저장하기 위해 `GameInstance`클래스에 변수를 선언했습니다.
```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpartaGameInstance.generated.h"

UCLASS()
class SPARTAPROJECT_API USpartaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USpartaGameInstance();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	int32 TotalScore;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	int32 CurrentLevelIndex;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	float TotalTime;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	int32 CollectCoin;

	UFUNCTION(BlueprintCallable, Category ="GameData")
	void AddToScore(int32 Amount);
	void AddToCollectCoin(int32 Amount);
	void AddToTotalTime(float Amount);

};

```
`GameInstance.h`
```cpp
#include "SpartaGameInstance.h"

USpartaGameInstance::USpartaGameInstance()
{
	TotalScore = 0;
	CurrentLevelIndex = 0;
	TotalTime = 0.0f;
	CollectCoin = 0;
}

void USpartaGameInstance::AddToScore(int32 Amount)
{
	TotalScore += Amount;
}

void USpartaGameInstance::AddToCollectCoin(int32 Amount)
{
	CollectCoin += Amount;
}

void USpartaGameInstance::AddToTotalTime(float Amount)
{
	TotalTime += Amount;
}

```
`GameInstance.cpp`

`GameInstance`에 저장되어있는 `CollectCoin`과 `TotalTime`에 값을 더해주기 위해서는 `GameState`클래스를 수정해주어야합니다.
우선 코인은 그냥 코인을 먹을때만 하나씩더해주면 되기때문에 단순하게 기존에 `OnCoinCollected()`함수에다가 추가했습니다.
```cpp
void ASpartaGameState::OnCoinCollected()
{
	CollectedCoinCount++;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToCollectCoin(1);
		}
	}

	if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
	{
		float RemainingTime = LevelDuration - GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
			if (SpartaGameInstance)
			{
				SpartaGameInstance->AddToTotalTime(RemainingTime);
			}
		}

		GetWorldTimerManager().ClearTimer(WaveTimerHandle);
		

		TArray<UClass*> ItemClasses;
		if (ACoinItem::StaticClass()) ItemClasses.Add(ACoinItem::StaticClass());
		if (AHealingItem::StaticClass()) ItemClasses.Add(AHealingItem::StaticClass());
		if (AMineItem::StaticClass()) ItemClasses.Add(AMineItem::StaticClass());
		if (!GetWorld())
		{
			return;
		}

		for (UClass* ItemClass : ItemClasses)
		{
			if (!ItemClass) continue; // nullptr 방지

			TArray<AActor*> FoundItems;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ItemClass, FoundItems);

			for (AActor* Item : FoundItems)
			{
				if (Item)
				{
					Item->Destroy();
				}
			}
		}


		if (CurrentWave >= 3)
		{
			EndLevel();
		}
		else
		{
			CurrentWave++;
			StartLevel();
		}
	}
}
```
이제 문제는 시간인데요, 현재 게임흐름에서 시간을 더해줄 경우는 두가지가 있습니다.
1. 지정되어있는 웨이브 시간이 지났을때
2. 코인을 다먹었을 때
우선 1번부터 보면
```cpp
void ASpartaGameState::OnWaveTimeUP()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToTotalTime(LevelDuration);
		}
	}

	TArray<UClass*> ItemClasses;
	if (ACoinItem::StaticClass()) ItemClasses.Add(ACoinItem::StaticClass());
	if (AHealingItem::StaticClass()) ItemClasses.Add(AHealingItem::StaticClass());
	if (AMineItem::StaticClass()) ItemClasses.Add(AMineItem::StaticClass());
	if (!GetWorld())
	{
		return;
	}

	for (UClass* ItemClass : ItemClasses)
	{
		if (!ItemClass) continue; // nullptr 방지

		TArray<AActor*> FoundItems;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ItemClass, FoundItems);

		for (AActor* Item : FoundItems)
		{
			if (Item)
			{
				Item->Destroy();
			}
		}
	}

	if (CurrentWave >= 3)
	{
		EndLevel();
	}
	else
	{
		CurrentWave++;
		StartLevel();
	}
}
```
`OnWaveTimeUP()`함수에서 
```cpp
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToTotalTime(LevelDuration);
		}
	}
```
`GameInstance`를 가져오고 `AddToTotalTime`메서드를 이용해서 현재 설정되어있는 웨이브시간만큼 더해줍니다.
다음 두번째 경우인데요. 두 번째는 코인을 다먹었을때 더해줘야합니다.
```cpp
void ASpartaGameState::OnCoinCollected()
{
	CollectedCoinCount++;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToCollectCoin(1);
		}
	}

	if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
	{
		float RemainingTime = LevelDuration - GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
			if (SpartaGameInstance)
			{
				SpartaGameInstance->AddToTotalTime(RemainingTime);
			}
		}

		GetWorldTimerManager().ClearTimer(WaveTimerHandle);
		

		TArray<UClass*> ItemClasses;
		if (ACoinItem::StaticClass()) ItemClasses.Add(ACoinItem::StaticClass());
		if (AHealingItem::StaticClass()) ItemClasses.Add(AHealingItem::StaticClass());
		if (AMineItem::StaticClass()) ItemClasses.Add(AMineItem::StaticClass());
		if (!GetWorld())
		{
			return;
		}

		for (UClass* ItemClass : ItemClasses)
		{
			if (!ItemClass) continue; // nullptr 방지

			TArray<AActor*> FoundItems;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ItemClass, FoundItems);

			for (AActor* Item : FoundItems)
			{
				if (Item)
				{
					Item->Destroy();
				}
			}
		}


		if (CurrentWave >= 3)
		{
			EndLevel();
		}
		else
		{
			CurrentWave++;
			StartLevel();
		}
	}
}
```
`OnCoinCollected()`함수에서 `if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)` 웨이브에 스폰된 모든 코인을 다먹었을때인 조건문에 
```cpp
	float 
ElapsedTime = LevelDuration - GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
			if (SpartaGameInstance)
			{
				SpartaGameInstance->AddToTotalTime(ElapsedTime);
			}
		}
```
를 넣어줬습니다.
걸린시간이기때문에 RemainigTime을 설정된 웨이브 시간에서 빼줘야합니다.

![SpartaProject 프리뷰  NetMode_ Standalone 0  (64-bit_PC D3D SM6) 2025-02-13 15-25-27](https://github.com/user-attachments/assets/5e51e525-66c4-4554-a983-d7834621d0a3)
