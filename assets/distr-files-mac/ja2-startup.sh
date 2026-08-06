#!/bin/sh
#
# CPACK_BUNDLE_NAME = OldGameVM → MacOS/OldGameVM + Resources/*

BUNDLE="$(cd "$(dirname "$0")/../.." && pwd)"
RESOURCES="$BUNDLE/Contents/Resources"

# Prefer Frameworks/ (codesign-friendly); fall back to Resources/ if older package.
if [ -d "$BUNDLE/Contents/Frameworks/SDL2.framework" ]; then
	export DYLD_FRAMEWORK_PATH="$BUNDLE/Contents/Frameworks${DYLD_FRAMEWORK_PATH:+:$DYLD_FRAMEWORK_PATH}"
elif [ -d "$RESOURCES/SDL2.framework" ]; then
	export DYLD_FRAMEWORK_PATH="$RESOURCES${DYLD_FRAMEWORK_PATH:+:$DYLD_FRAMEWORK_PATH}"
fi

exec "$RESOURCES/ja2-launcher"
