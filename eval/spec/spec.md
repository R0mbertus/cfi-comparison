# chosen spec benches

```
fp: 508.namd_r 510.parest_r 511.povray_r 519.lbm_r 538.imagick_r 544.nab_r 997.specrand_fr
int: 520.omnetpp_r 531.deepsjeng_r 541.leela_r 557.xz_r 999.specrand_ir

runcpu --iterations=3 --reportable --config=<policy>.cfg \
    intrate fprate
```
