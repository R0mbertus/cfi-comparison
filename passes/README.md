# Passes

`clang++-15 -O0 -fpass-plugin=build/lib/libControlFlowIntegrity.so test/main.cpp -o test/main`

Nothing
OK: 517 SOME: 0 FAIL: 467 NP: 936 Total Attacks: 984

Base CFI
OK: 324 SOME: 0 FAIL: 660 NP: 936 Total Attacks: 984

Shadow
OK: 516 SOME: 0 FAIL: 468 NP: 936 Total Attacks: 984
