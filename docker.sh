#!/bin/bash
# Purpose:  Script to setup the docker container and/or enter it
# Usage:    ./setup.sh (flags)
# Default:  Checks if docker exists and is running, if not creates/starts it
# ---------------------------------------

force_build=false
enter=false
container="bachelor-thesis"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo " -b   Force a rebuild of the docker"
    echo " -d   Disable detached start"
    echo " -h   Display this help message"
}

enter_container() {
    echo "[--] Entering container..."
    docker exec -it $container /bin/bash
    exit 0
}

while getopts ":beh" flag; do
    case $flag in
        b) # Force a rebuild
            force_build=true
        ;;
        e) # Enter the docker
            enter=true
        ;;
        h) # Display usage
            usage
            exit 0
        ;;
    esac
done

# Sanity check; is docker & co installed?
if ! command -v docker &> /dev/null;
then
    echo "[:(] docker is not installed."
    exit 1
fi

if [ "$force_build" == false ] && \
    docker compose ps $container | grep Up &> /dev/null;
then
    [ "$enter" == true ] && enter_container
    echo "[:(] Container is already running, did you mean to run with -e?"
    exit 0
fi

# Build and start the docker container
echo "[--] Building container, may take a while..."
docker compose build &> /dev/null
echo "[:)] Docker container built!"
echo "[--] Starting container..."
docker compose up -d &> /dev/null
echo "[:)] Docker container up & running!"

[ "$enter" == true ] && enter_container
