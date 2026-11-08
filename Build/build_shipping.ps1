# Requires UE 5.5 (set $env:UE_ROOT, e.g. "C:\Program Files\Epic Games\UE_5.5"). NOT YET RUN: no UE install available in authoring environment.
param([string]$UERoot=$env:UE_ROOT)
$proj="$PSScriptRoot\..\Afterclass.uproject"
& "$UERoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="$proj" -platform=Win64 -clientconfig=Shipping -serverconfig=Shipping -cook -allmaps -stage -pak -package -prereqs -buildmachine -nop4 -utf8output
if($LASTEXITCODE -ne 0){ exit $LASTEXITCODE }
