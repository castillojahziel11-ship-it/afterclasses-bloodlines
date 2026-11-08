#include "Characters/ACCharacterBase.h"
#include "Afterclass.h"
#include "Characters/ACCharacterDataAsset.h"
#include "Combat/ACCombatComponent.h"
#include "Input/ACInputSettings.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"

AACCharacterBase::AACCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->MaxWalkSpeed = DefaultWalkSpeed;
	Move->BrakingDecelerationWalking = 2048.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Combat = CreateDefaultSubobject<UACCombatComponent>(TEXT("Combat"));
}

void AACCharacterBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCharacterData(); // preview mesh in editor viewport
}

void AACCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	ApplyCharacterData();
}

void AACCharacterBase::ApplyCharacterData()
{
	WalkSpeed = DefaultWalkSpeed;
	SprintSpeed = DefaultSprintSpeed;

	if (CharacterData)
	{
		WalkSpeed = CharacterData->WalkSpeed;
		SprintSpeed = CharacterData->SprintSpeed;

		if (USkeletalMesh* Body = CharacterData->BodyMesh.LoadSynchronous()) // STUB: switch to async streaming via AssetManager (KI-006)
		{
			GetMesh()->SetSkeletalMeshAsset(Body);
		}
		if (UClass* AnimClass = CharacterData->AnimBP.LoadSynchronous())
		{
			GetMesh()->SetAnimInstanceClass(AnimClass);
		}
		if (Combat && HasAuthority())
		{
			Combat->InitializeVitals(CharacterData->MaxHealth, CharacterData->MaxStamina);
		}
	}

	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

UInputAction* AACCharacterBase::LoadAction(const TSoftObjectPtr<UInputAction>& Ref, const TCHAR* Label)
{
	UInputAction* Action = Ref.LoadSynchronous();
	if (!Action)
	{
		UE_LOG(LogAfterclass, Error, TEXT("Input action '%s' missing at '%s'. Create it in the editor (Docs/EDITOR_SETUP.md) or fix Project Settings > Game > Afterclass Input."),
			Label, *Ref.ToSoftObjectPath().ToString());
	}
	return Action;
}

void AACCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogAfterclass, Error, TEXT("Input component is not UEnhancedInputComponent. Check Config/DefaultInput.ini DefaultInputComponentClass."));
		return;
	}

	const UACInputSettings* S = UACInputSettings::Get();

	if (UInputAction* A = LoadAction(S->MoveAction, TEXT("Move")))
	{
		EIC->BindAction(A, ETriggerEvent::Triggered, this, &AACCharacterBase::Input_Move);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AACCharacterBase::Input_Move);
	}
	if (UInputAction* A = LoadAction(S->LookAction, TEXT("Look")))    { EIC->BindAction(A, ETriggerEvent::Triggered, this, &AACCharacterBase::Input_Look); }
	if (UInputAction* A = LoadAction(S->LightAction, TEXT("Light")))  { EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_Light); }
	if (UInputAction* A = LoadAction(S->HeavyAction, TEXT("Heavy")))  { EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_Heavy); }
	if (UInputAction* A = LoadAction(S->DodgeAction, TEXT("Dodge")))  { EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_Dodge); }
	if (UInputAction* A = LoadAction(S->GrabAction, TEXT("Grab")))    { EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_Grab); }
	if (UInputAction* A = LoadAction(S->SpecialAction, TEXT("Special"))) { EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_Special); }
	if (UInputAction* A = LoadAction(S->BlockAction, TEXT("Block")))
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_BlockStarted);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AACCharacterBase::Input_BlockCompleted);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &AACCharacterBase::Input_BlockCompleted);
	}
	if (UInputAction* A = LoadAction(S->SprintAction, TEXT("Sprint")))
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &AACCharacterBase::Input_SprintStarted);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AACCharacterBase::Input_SprintCompleted);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &AACCharacterBase::Input_SprintCompleted);
	}
}

void AACCharacterBase::Input_Move(const FInputActionValue& Value)
{
	LastMoveInput = Value.Get<FVector2D>();
	if (!Controller || LastMoveInput.IsNearlyZero()) return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, LastMoveInput.Y);
	AddMovementInput(Right, LastMoveInput.X);
}

void AACCharacterBase::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Look = Value.Get<FVector2D>();
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y); // invert-Y is applied via a Negate modifier from profile settings (KI-005)
}

void AACCharacterBase::Input_Light(const FInputActionValue&)   { if (Combat) Combat->RequestLight(); }
void AACCharacterBase::Input_Heavy(const FInputActionValue&)   { if (Combat) Combat->RequestHeavy(); }
void AACCharacterBase::Input_Dodge(const FInputActionValue&)   { if (Combat) Combat->RequestDodge(LastMoveInput); }
void AACCharacterBase::Input_Grab(const FInputActionValue&)    { if (Combat) Combat->RequestGrab(); }
void AACCharacterBase::Input_Special(const FInputActionValue&) { if (Combat) Combat->RequestSpecial(); }
void AACCharacterBase::Input_BlockStarted(const FInputActionValue&)   { if (Combat) Combat->RequestBlock(true); }
void AACCharacterBase::Input_BlockCompleted(const FInputActionValue&) { if (Combat) Combat->RequestBlock(false); }
void AACCharacterBase::Input_SprintStarted(const FInputActionValue&)   { SetSprinting(true); }
void AACCharacterBase::Input_SprintCompleted(const FInputActionValue&) { SetSprinting(false); }

void AACCharacterBase::SetSprinting(bool bSprinting)
{
	bIsSprinting = bSprinting;
	// STUB: MaxWalkSpeed is set locally on the owning client and is not yet server-replicated/predicted (KI-008).
	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
	if (Combat) Combat->SetSprinting(bSprinting);
}
