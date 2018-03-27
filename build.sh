#!/usr/bin/env bash


export objDir="$PWD/obj"
export binDir="$PWD/bin/"
export currentDir="$PWD"

export __CMakeBuildType="debug"
export __CMakeBinDir="$binDir"
echo "$__CMakeBinDir"

if [ ! -d "$objDir" ]; then 
	mkdir "$objDir"
fi

if [ ! -d "$binDir" ]; then 
	mkdir "$binDir"
fi


while :; do
    if [ $# -le 0 ]; then
        break
    fi

    lowerI="$(echo $1 | awk '{print tolower($0)}')"
    case $lowerI in
        -\?|-h|--help)
            usage
            exit 1
            ;;

        debug|-debug)
            __CMakeBuildType=Debug
            ;;

        checked|-checked)
            __CMakeBuildType=Checked
            ;;

        release|-release)
            __CMakeBuildType=Release
            ;;

        *)
            __UnprocessedBuildArgs="$__UnprocessedBuildArgs $1"
            ;;
    esac

    shift
done

pushd $objDir
cmake "..\\"
rc=$?
if [[ $rc != 0 ]]; then 
	popd;
	exit $rc;
fi


make
rc=$?
if [[ $rc != 0 ]]; then 
	popd;
	exit $rc;
fi

make install
rc=$?
if [[ $rc != 0 ]]; then 
	popd;
	exit $rc;
fi


popd
