using UnrealBuildTool;
public class Afterclass : ModuleRules {
  public Afterclass(ReadOnlyTargetRules Target) : base(Target) {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","EnhancedInput","GameplayTags","GameplayAbilities","AIModule","UMG","CommonUI","OnlineSubsystem","NetCore"});
  }
}
