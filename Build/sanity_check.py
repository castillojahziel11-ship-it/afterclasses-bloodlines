#!/usr/bin/env python3
"""Best-effort static checks for UE C++ conventions. NOT a substitute for compiling with UBT.
Checks: generated.h is the last include and matches the file name; GENERATED_BODY in every reflected type;
API macro on exported classes; Replicated props => GetLifetimeReplicatedProps defined; Server/Client/NetMulticast RPCs
have _Implementation; balanced braces/parens; .cpp includes its own header first; no FText::FromString( anywhere in Source."""
import re, sys, pathlib
root = pathlib.Path(__file__).resolve().parent.parent / "Source"
errors = []
def err(f, m): errors.append(f"{f.relative_to(root.parent)}: {m}")
def strip(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S); s = re.sub(r'//[^\n]*', '', s)
    return re.sub(r'"(\\.|[^"\\])*"', '""', s)
for f in sorted(root.rglob("*.[hc]*")):
    if f.suffix not in (".h", ".cpp"): continue
    raw = f.read_text(); code = strip(raw)
    for a, b in ("{}", "()"):
        if code.count(a) != code.count(b): err(f, f"unbalanced {a}{b}: {code.count(a)} vs {code.count(b)}")
    if f.suffix == ".h":
        reflected = re.findall(r'\b(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(', code)
        types = re.findall(r'\b(?:UCLASS|USTRUCT)\s*\([^)]*\)\s*(?:class|struct)\s+(\w+)\s+(\w+)', code)
        if reflected and any(r != "UENUM" for r in reflected) or "UENUM" in reflected:
            incs = re.findall(r'#include\s+"([^"]+)"', re.sub(r'//[^\n]*', '', raw))
            gen = f"{f.stem}.generated.h"
            if not incs or incs[-1] != gen: err(f, f"last include must be {gen} (got {incs[-1] if incs else None})")
        n_bodies = len(re.findall(r'\bGENERATED_(?:UCLASS_)?BODY\s*\(', code))
        n_types = len(re.findall(r'\b(?:UCLASS|USTRUCT|UINTERFACE)\s*\(', code))
        if n_bodies != n_types: err(f, f"{n_types} reflected types but {n_bodies} GENERATED_BODY")
        for api, name in types:
            if api != "AFTERCLASS_API" and "Source/Afterclass/" in str(f): err(f, f"{name} missing AFTERCLASS_API")
        if re.search(r'UPROPERTY\s*\([^)]*\bReplicated', code):
            cpp = f.with_suffix(".cpp")
            c = cpp.read_text() if cpp.exists() else ""
            if "GetLifetimeReplicatedProps" not in code or "GetLifetimeReplicatedProps" not in c: err(f, "Replicated props without GetLifetimeReplicatedProps")
            if "Net/UnrealNetwork.h" not in c: err(cpp, "missing #include \"Net/UnrealNetwork.h\"")
        if re.search(r'UPROPERTY\s*\([^)]*\)\s*[\w:<>]+\s+\w+\s*(=[^;,(]*(\([^)]*\))?)?\s*,', code): err(f, "multiple declarators in one UPROPERTY")
        for rpc in re.findall(r'UFUNCTION\s*\([^)]*\b(?:Server|Client|NetMulticast)\b[^)]*\)\s*void\s+(\w+)', code):
            cpp = f.with_suffix(".cpp")
            if not cpp.exists() or f"{rpc}_Implementation" not in cpp.read_text(): err(f, f"RPC {rpc} has no _Implementation")
        if "#pragma once" not in raw: err(f, "missing #pragma once")
    else:
        incs = re.findall(r'#include\s+"([^"]+)"', re.sub(r'//[^\n]*', '', raw))
        hdr = f.with_suffix(".h")
        if hdr.exists() and (not incs or pathlib.Path(incs[0]).name != hdr.name): err(f, f"first include should be its own header {hdr.name}")
    if re.search(r"FText::FromString\s*\(", code): err(f, "FText::FromString banned (use ACText / string tables)")
print("\n".join(errors) if errors else "sanity_check: OK (static only; not a UE compile)")
sys.exit(1 if errors else 0)
