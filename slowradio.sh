#! /bin/bash

CONTROL_SH=$(basename "$0")
oneline_usage="$CONTROL_SH [-h] command [args]"

usage()
{
    cat <<-EndUsage
		Usage: $oneline_usage
		Use '$CONTROL_SH -h' for more information
	EndUsage
    exit 1
}

helpinfo()
{
cat <<-EndHelp
Usage: $oneline_usage

Commands:
    build [tag]
        Build a new docker image for a given tag
    push [tag]
        Push latest version up to docker hub
    pull [tag]
        Pull latest version from docker hub
    foreground optfolder [tag] [name]
        Run in the foreground
    run optfolder [tag] [name]
        Run in the background
    connect [name]
        Connect to a bash session on a running container
    stats [name]
        Stats on running process

    [name] always defaults to *slowradio*
    [tag] always defaults to *latest*

Flags:
    -h
        Show usage info

EndHelp
exit 0
}

die()
{
    echo "$*"
    exit 1
}

readonly REPO="rumblesan"
readonly APP="slowradio"

build()
{
    echo "Building new $REPO/$APP image with tag $DOCKER_TAG"
    docker build -t $REPO/$APP:$DOCKER_TAG .
}

push()
{
    echo "Pushing new $REPO/$APP image with tag $DOCKER_TAG"
    docker push $REPO/$APP:$DOCKER_TAG
}

pull()
{
    echo "Pulling new $REPO/$APP image with tag $DOCKER_TAG"
    docker pull $REPO/$APP:$DOCKER_TAG
}

foreground()
{
    if [ -z "$1" ]; then
        dir "Need to give opt folder"
    fi
    local optfolder="$1"
    echo "Running $APP in the foreground"
    docker run --name $CONTAINER_NAME -v "$1":/opt/slowradio $REPO/$APP:$DOCKER_TAG 
}

run()
{
    if [ -z "$1" ]; then
        dir "Need to give opt folder"
    fi
    local optfolder="$1"
    echo "Running $APP"
    docker run --name $CONTAINER_NAME -d -v "$1":/opt/slowradio $REPO/$APP:$DOCKER_TAG 
}

connect()
{
    echo "Connecting to $CONTAINER_NAME"
    docker exec -it $CONTAINER_NAME bash
}

stats()
{
    echo "Connecting to $CONTAINER_NAME"
    docker exec $CONTAINER_NAME ps up $(docker exec $CONTAINER_NAME pgrep -f 'slow')
}

runaction()
{
    if [ ! -f Dockerfile ]; then
        die "This script needs to be run in the same directory as the Dockerfile"
    fi

    action=$( printf "%s\n" "$1" | tr 'A-Z' 'a-z' )

    case "$action" in
    "build" )
        build
        ;;
    "push" )
        push
        ;;
    "pull" )
        push
        ;;
    "foreground" )
        foreground
        ;;
    "run" )
        run
        ;;
    "connect" )
        connect
        ;;
    "stats" )
        stats
        ;;
    * )
        usage
        ;;
    esac

}

echo "$APP control"

CONTAINER_NAME="slowradio"
DOCKER_TAG="latest"

while getopts "h:n::t:" opt "$@"; do
    case "$opt" in
        h)
            helpinfo
            ;;
        n)
            CONTAINER_NAME="$OPTARG"
            echo "Using docker tag $CONTAINER_NAME"
            ;;
        t)
            DOCKER_TAG="$OPTARG"
            echo "Using docker tag $DOCKER_TAG"
            ;;
    esac
done
runaction "${@:$OPTIND}"

