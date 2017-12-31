#!/bin/sh

cd src/protobuf
protoc --cpp_out=. cache_index.proto
protoc --cpp_out=. labels.proto
protoc --cpp_out=. vector_tile.proto
protoc --cpp_out=. xyz.proto
cd ../..

