#include "SpartaGameState.h"
#include "SpartaGameInstance.h"
#include "SpartaCharacter.h"
#include "SpartaPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SpawnVolume.h"
#include "CoinItem.h"
#include "MineItem.h"
#include "HealingItem.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"

ASpartaGameState::ASpartaGameState()
{
	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	LevelDuration = 30.0f;
	CurrentLevelIndex = 0;
	CurrentWave = 1;
	MaxLevels = 3;
	bIsClear = false;
}

void ASpartaGameState::BeginPlay()
{
	Super::BeginPlay();

	StartLevel();

	GetWorldTimerManager().SetTimer(
		HUDUpdateTimerHandle,
		this,
		&ASpartaGameState::UpdateHUD,
		0.1f,
		true
	);
}

int32 ASpartaGameState::GetScore() const
{
	return Score;
}

void ASpartaGameState::AddScore(int32 Amount)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToScore(Amount);
		}
	}
}

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

	GetWorldTimerManager().SetTimer(
		WaveTimerHandle,
		this,
		&ASpartaGameState::OnWaveTimeUP,
		LevelDuration,
		false
	);

}

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

void ASpartaGameState::EndLevel()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			AddScore(Score);
			CurrentLevelIndex++;
			CurrentWave = 1;
			SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;
		}
	}

	if (CurrentLevelIndex >= MaxLevels)
	{
		bIsClear = true;
		OnGameOver();
		return;
	}

	if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
	{
		UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
	}
	else
	{
		OnGameOver();
	}
}

void ASpartaGameState::OnGameOver()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			CurrentWave = 1;
			SpartaPlayerController->SetPause(true);

			if (bIsClear)
			{
				SpartaPlayerController->ShowClearMenu();
			}
			else
			{
				SpartaPlayerController->ShowGameOverMenu();
			}
			
		}
	}
}

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
		float ElapsedTime = LevelDuration - GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
			if (SpartaGameInstance)
			{
				SpartaGameInstance->AddToTotalTime(ElapsedTime);
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

void ASpartaGameState::UpdateHUD()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			if (UUserWidget* HUDWidget = SpartaPlayerController->GetHUDWidget())
			{
				if (UTextBlock* TimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
				{
					float RemainingTime = GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
					TimeText->SetText(FText::FromString(FString::Printf(TEXT("Time: %.1f"), RemainingTime)));
				}

				if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
				{
					if (UGameInstance* GameInstance = GetGameInstance())
					{
						USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
						if (SpartaGameInstance)
						{
							ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), SpartaGameInstance->TotalScore)));
						}
					}
				}

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
			}
		}
	}
}