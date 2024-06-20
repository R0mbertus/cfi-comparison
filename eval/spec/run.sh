#!/bin/bash

docker exec -it main bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=clang-base.cfg --noreportable intspeed fpspeed"

docker exec -it main bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=clang-cfi.cfg --noreportable intspeed fpspeed"

docker exec -it main bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=clang-kcfi.cfg --noreportable intspeed fpspeed"

docker exec -it main bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=gcc-cfi.cfg --noreportable intspeed fpspeed"

docker exec -it custom bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=clang-type.cfg --noreportable intspeed fpspeed"

docker exec -it custom bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=clang-opaque.cfg --noreportable intspeed fpspeed"

docker exec -it typro bash -c "cd /mnt/spec && source shrc && \
    runcpu --iterations=3 --config=clang-typro.cfg --noreportable intspeed fpspeed"
