@echo off
setlocal

pushd bin\

test_runner.exe %*

popd
