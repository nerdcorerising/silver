#!/usr/bin/env bash


export objDir="$PWD/obj"
export binDir="$PWD/bin/"
export currentDir="$PWD"
export programsDir=$binDir/programs
export frameworkDir=$binDir/framework

export __CMakeBuildType="debug"
export __CMakeBinDir="$binDir"
echo "$__CMakeBinDir"

if [ ! -d "$objDir" ]; then 
	mkdir "$objDir"
fi

if [ ! -d "$binDir" ]; then 
	mkdir "$binDir"
fi

if [ ! -d "$programsDir" ]; then 
    mkdir -p "$programsDir"
fi

if [ ! -d "$frameworkDir" ]; then 
    mkdir -p "$frameworkDir"
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


make -j12
rc=$?
if [[ $rc != 0 ]]; then 
	popd;
	exit $rc;
fi

popd

echo Copying binaries
cp -f $objDir/src/compiler/silver $binDir/
cp -f src/compiler/framework/* $frameworkDir/
cp -f src/test/programs/* $programsDir/
cp -f src/test/*.py $binDir/
