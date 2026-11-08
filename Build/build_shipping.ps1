# Requires UE 5.4+ (set $env:UE_ROOT). NOT YET RUN: no UE install available in authoring environment.
param([string]$UERoot=$env:UE_ROOT)
$proj="$PSScriptRoot\..\Afterclass.uproject"
& "$UERoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="$proj" -platform=Win64 -clientconfig=Shipping -serverconfig=Shipping -cook -allmaps -stage -pak -package -prereqs -buildmachine -nop4 -utf8output
if($LASTEXITCODE -ne 0){ exit $LASTEXITCODE }
