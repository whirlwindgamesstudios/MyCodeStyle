// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameModeBase.h"

#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MapObject/Character/MainCharacter.h"
#include "MapObject/Character/MainPlayerController.h"
#include "MapObject/GamePlayObject/CameraPawn.h"
#include "MapObject/MiniGame/MiniGamePawn.h"
#include "MapObject/Points/PointSpawnClient.h"
#include "MapObject/Points/Spawn_Points_Of_Prostitutes.h"

AMainGameModeBase::AMainGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMainGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> SwitchCameras;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "SwitchCamera", SwitchCameras);
	if (SwitchCameras.Num() != 0)ChoiceCamera = Cast<ACameraPawn>(SwitchCameras[0]);
	PlayerController = Cast<AMainPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}

void AMainGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckDistanceSwitchCamera();
}

void AMainGameModeBase::SpawnNewClient()
{
	FVector SpawnClientLocation;
	FRotator SpawnClientRotation;
	TArray<AActor*> SpawnPointArray;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APointSpawnClient::StaticClass(), SpawnPointArray);
	for (auto PointArray : SpawnPointArray)
	{
		SpawnClientLocation = PointArray->GetActorLocation();
		SpawnClientRotation = PointArray->GetActorRotation();
	}
	FActorSpawnParameters SpawnInfo;
	ClientCharacter = GetWorld()->SpawnActor<ACharacter>(*ClassClientCharacter, SpawnClientLocation,
	                                                     SpawnClientRotation, SpawnInfo);
	SetupClientParametrs(ClientCharacter);
	CheckDistanceActive = true;
	SpawnProstitutesForClient();
}

void AMainGameModeBase::SwitchCamera()
{
	TArray<AActor*> AllProstituteSpawnPointForClient;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ProstituteSpawnPoint", AllProstituteSpawnPointForClient);
	for (auto SpawnPoint : AllProstituteSpawnPointForClient)
	{
		ASpawn_Points_Of_Prostitutes* Spawn_Points_Of_Prostitutes = Cast<ASpawn_Points_Of_Prostitutes>(SpawnPoint);
		if (Spawn_Points_Of_Prostitutes != nullptr && Spawn_Points_Of_Prostitutes->ZoneFree == true)
		{
			PlayerController->GetPawn()->SetActorLocation(SpawnPoint->GetActorLocation());
			PlayerController->GetPawn()->SetActorRotation(SpawnPoint->GetActorRotation());
			Spawn_Points_Of_Prostitutes->ZoneFree = false;
		}
	}
	PlayerController->GetPawn()->DisableInput(PlayerController);
	PlayerController->GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 0;
	PlayerController->GetCharacter()->GetMesh()->GetAnimInstance()->StopSlotAnimation();
	PlayerController->bShowMouseCursor = true;
	PlayerController->ChoiceActive = true;

	PlayerController->Possess(ChoiceCamera);
	CheckDistanceActive = false;
}

void AMainGameModeBase::CheckDistanceSwitchCamera()
{
	if (CheckDistanceActive)
		if (PlayerController != nullptr && ClientCharacter != nullptr)
		{
			FVector PlayerLocation = PlayerController->GetPawn()->GetActorLocation();
			FVector PointSpawnClientLocation = ClientCharacter->GetActorLocation();

			float Distance = sqrt(
				powf(PlayerLocation.X - PointSpawnClientLocation.X, 2) + powf(
					PlayerLocation.Y - PointSpawnClientLocation.Y, 2) + powf(
					PlayerLocation.Z - PointSpawnClientLocation.Z, 2));

			if (Distance < ActivateSwitchCameraDistance)
			{
				PlayerCharacter = Cast<AMainCharacter>(PlayerController->GetPawn());
				PlayerCharacter->IsFree = true;
				SwitchCamera();
			}
		}
}

void AMainGameModeBase::RandomSpawnNewClient()
{
	GetWorldTimerManager().SetTimer(RandomSpawnNewClientHandle, this, &AMainGameModeBase::SpawnNewClient,
	                                RandomDelaySpawnClient, false);
}


void AMainGameModeBase::SpawnProstitutesForClient()
{
	TArray<AActor*> AllProstituteSpawnPointForClient;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ProstituteSpawnPoint", AllProstituteSpawnPointForClient);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Prostitute", AllProstitute);
	int Index = 0;
	for (auto ProstituteActor : AllProstitute)
		if (AllProstituteSpawnPointForClient.IsValidIndex(Index))
		{
			ASpawn_Points_Of_Prostitutes* Spawn_Points_Of_Prostitutes = Cast<ASpawn_Points_Of_Prostitutes>(
				AllProstituteSpawnPointForClient[Index]);
			AMainCharacter* ProstituteCharacter = Cast<AMainCharacter>(ProstituteActor);
			if (ProstituteCharacter->IsFree == true && Spawn_Points_Of_Prostitutes->ZoneFree != false)
			{
				Spawn_Points_Of_Prostitutes->ZoneFree = false;
				ProstituteCharacter->SetActorTransform(AllProstituteSpawnPointForClient[Index]->GetActorTransform());
			}
			Index++;
		}
}

void AMainGameModeBase::RespawnProstitutes()
{
	TArray<FTransform> ActorTransform;

	TArray<AActor*> AllProstituteReSpawnPoint;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ProstituteReSpawnPoint", AllProstituteReSpawnPoint);

	TArray<AActor*> AllProstituteSpawnPointForClient;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ProstituteSpawnPoint", AllProstituteSpawnPointForClient);

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Prostitute", AllProstitute);
	for (auto SpawnPoint : AllProstituteSpawnPointForClient)
	{
		ASpawn_Points_Of_Prostitutes* Spawn_Points_Of_Prostitutes = Cast<ASpawn_Points_Of_Prostitutes>(SpawnPoint);
		Spawn_Points_Of_Prostitutes->ZoneFree = true;
	}
	for (auto ProstituteReSpawnPoint : AllProstituteReSpawnPoint)
		ActorTransform.Add(ProstituteReSpawnPoint->GetActorTransform());
	int Index = 0;
	for (auto ProstituteActor : AllProstitute)
		if (ActorTransform.IsValidIndex(Index))
		{
			AMainCharacter* ProstituteCharacter = Cast<AMainCharacter>(ProstituteActor);
			APlayerController* ActorPlayerController = Cast<APlayerController>(ProstituteCharacter->GetController());
			ProstituteCharacter->EnableInput(ActorPlayerController);
			ProstituteCharacter->GetCharacterMovement()->MaxWalkSpeed = 300;
			if (ProstituteCharacter->IsFree == true)ProstituteCharacter->SetActorTransform(ActorTransform[Index]);
			Index++;
		}
}

void AMainGameModeBase::StartMiniGame(AMainPlayerController* Controller, AActor* Bed)
{
	PlayerCharacter = Cast<AMainCharacter>(Controller->GetPawn());
	CharacterController = Controller;
	BedActor = Bed;
	FActorSpawnParameters SpawnInfo;
	switch (PlayerCharacter->TypeMiniGame)
	{
	case eMiniGameTypeClass::FlappyBirdType:
		MiniGamePawn = GetWorld()->SpawnActor<AMiniGamePawn>(*ClassMiniGamePawnFlappy,
		                                                     FVector(3435.f, -10425.f, 5370.f),
		                                                     FRotator(0.f, 180.f, 0.f), SpawnInfo);
		break;
	case eMiniGameTypeClass::RunnerType:
		MiniGamePawn = GetWorld()->SpawnActor<AMiniGamePawn>(*ClassMiniGamePawnRunner,
		                                                     FVector(3717.f, -10223.f, 15514.f),
		                                                     FRotator(0.f, 180.f, 0.f), SpawnInfo);
		break;
	default:
		MiniGamePawn = GetWorld()->SpawnActor<AMiniGamePawn>(*ClassMiniGamePawnFlappy,
		                                                     FVector(3435.f, -10425.f, 5370.f),
		                                                     FRotator(0.f, 180.f, 0.f), SpawnInfo);
		break;
	}
	CharacterController->Possess(MiniGamePawn); //Переселяем котроллер в пешку для миниигры
	CharacterController->SetViewTarget(BedActor); //Ставим вид камеры на кровать.
}

void AMainGameModeBase::FinishMiniGame(float MentalPoint)
{
	CharacterController->Possess(PlayerCharacter); //Переселяемся в своего персонажа
	PlayerCharacter->LeadsTheClient = false;
	PlayerCharacter->PlusMentalState(MentalPoint); //По завершению игры добавляем очки осознанности к персонажу.
	PlayerCharacter->EnableInput(CharacterController);
	PlayerCharacter->PlusMoney(MonetaryReward);
	CharacterController->InteractiveActive = true;
	//По завершению мини игры игрок снова может взаимодействовать с предметами.
	PlayerCharacter = nullptr;
	CharacterController = nullptr;
	BedActor = nullptr;
	MiniGamePawn->Destroy();
	if (ClientCharacter != nullptr)
	{
		ClientCharacter->Destroy();
		ClientCharacter = nullptr;
	}
	ProstituteCharacterActor = nullptr;
}
