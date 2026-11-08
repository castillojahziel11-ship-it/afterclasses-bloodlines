// STUB: see header / KI-001.
#include "ACCombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UACCombatComponent::UACCombatComponent()
{
        PrimaryComponentTick.bCanEverTick = false;
        SetIsReplicatedByDefault(true);
}

void UACCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UACCombatComponent, State);
        DOREPLIFETIME(UACCombatComponent, Health);
        DOREPLIFETIME(UACCombatComponent, MaxHealth);
        DOREPLIFETIME(UACCombatComponent, Stamina);
        DOREPLIFETIME(UACCombatComponent, MaxStamina);
        DOREPLIFETIME(UACCombatComponent, Meter);
}

void UACCombatComponent::InitializeVitals(float InMaxHealth, float InMaxStamina)
{
        MaxHealth = FMath::Max(1.f, InMaxHealth);
        MaxStamina = FMath::Max(0.f, InMaxStamina);
        Health = MaxHealth;
        Stamina = MaxStamina;
        Meter = AC::ClampMeter(0.f);
}

bool UACCombatComponent::IsInFreeState() const
{
        return State == EACCombatState::Idle || State == EACCombatState::Locomotion || State == EACCombatState::Sprint;
}

bool UACCombatComponent::TryStartAction(EACCombatState Next, FName MoveId)
{
        if (State == EACCombatState::Dead) return false;
        if (!(IsInFreeState() || bCancelWindowOpen)) return false; // legality + cancel window
        // STUB: stamina cost lookup from the fighter MoveSet (KI-001).
        if (bActionLocked) return false;
        // STUB: facing correction and montage dispatch for MoveId (KI-001).
        SetState(Next);
        return true;
}

void UACCombatComponent::SetState(EACCombatState NewState)
{
        if (NewState == State) return;
        const EACCombatState Old = State;
        State = NewState;
        OnCombatStateChanged.Broadcast(Old, State);
}

void UACCombatComponent::OnRep_State(EACCombatState OldState)
{
        OnCombatStateChanged.Broadcast(OldState, State);
}

void UACCombatComponent::RouteAction(EACCombatState Next, FName MoveId)
{
        const AActor* Owner = GetOwner();
        if (!Owner) return;
        if (Owner->HasAuthority())
        {
                TryStartAction(Next, MoveId);
        }
        else
        {
                ServerRequestAction(Next, MoveId);
        }
}

void UACCombatComponent::ServerRequestAction_Implementation(EACCombatState Next, FName MoveId)
{
        if (MoveId == FName(TEXT("BlockRelease")))
        {
                if (State == EACCombatState::Blocking) { SetState(EACCombatState::Idle); }
                return;
        }
        TryStartAction(Next, MoveId);
}

void UACCombatComponent::RequestLight()   { RouteAction(EACCombatState::LightAttack, TEXT("Light")); }
void UACCombatComponent::RequestHeavy()   { RouteAction(EACCombatState::HeavyAttack, TEXT("Heavy")); }
void UACCombatComponent::RequestDodge(FVector2D /*Dir*/) { RouteAction(EACCombatState::Dodging, TEXT("Dodge")); } // STUB: dodge direction unused (KI-001)
void UACCombatComponent::RequestGrab()    { RouteAction(EACCombatState::Grabbing, TEXT("Grab")); }
void UACCombatComponent::RequestSpecial() { RouteAction(EACCombatState::Special, TEXT("Special")); }

void UACCombatComponent::RequestBlock(bool bHeld)
{
        if (bHeld)
        {
                RouteAction(EACCombatState::Blocking, TEXT("Block"));
        }
        else if (State == EACCombatState::Blocking)
        {
                // Releasing block always returns to Idle, bypassing the free-state legality check.
                if (GetOwner() && GetOwner()->HasAuthority()) { SetState(EACCombatState::Idle); }
                else { ServerRequestAction(EACCombatState::Idle, TEXT("BlockRelease")); }
        }
}

void UACCombatComponent::SetSprinting(bool bSprinting)
{
        if (bSprinting && IsInFreeState())
        {
                RouteAction(EACCombatState::Sprint, TEXT("Sprint"));
        }
        else if (!bSprinting && State == EACCombatState::Sprint)
        {
                RouteAction(EACCombatState::Locomotion, TEXT("SprintEnd"));
        }
}
