#!/bin/bash

set -e

version=$(node ./test/integration/test.cjs -v --vizzu /dist/vizzu.min.js|awk -F'-' '{print $1}')
echo "version: $version"
npm install vizzu@$version
node ./tools/ci/purge.cjs $version
