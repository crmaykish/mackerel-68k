#!/bin/sh

docker build -t mackerel-68k .
docker run -it -v ${1}:/home/mackerel/project mackerel-68k