#!/usr/bin/env bash

build_dir="$(pwd)/build"

VERSION=$(cat res/version.txt)
if [[ "$TRAVIS_BRANCH" == "master" ]]
then
  REPO=julius
  NAME_SUFFIX=-unstable
else
  REPO=julius-branches
  NAME_SUFFIX=
  VERSION=${TRAVIS_BRANCH##feature/}-$VERSION
fi

# Linux portable binary: https://appimage.org/
if [ "$DEPLOY" = "appimage" ]
then
cat > "bintray.json" <<EOF
{
  "package": {
    "subject": "keriew",
    "repo": "$REPO",
    "name": "linux$NAME_SUFFIX",
    "licenses": ["AGPL-V3"],
    "vcs_url": "https://github.com/keriew/julius.git"
  },

  "version": {
    "name": "$VERSION",
    "released": "$(date +'%Y-%m-%d')",
    "desc": "Automated Linux AppImage build for Travis-CI job: $TRAVIS_JOB_WEB_URL"
  },

  "files": [
    {
      "includePattern": "${build_dir}/juliusgc.AppImage",
      "uploadPattern": "juliusgc-$VERSION-linux.AppImage"
    }
  ],

  "publish": true
}
EOF
elif [ "$DEPLOY" = "mac" ]
then
cat > "bintray.json" <<EOF
{
  "package": {
    "subject": "keriew",
    "repo": "$REPO",
    "name": "mac$NAME_SUFFIX",
    "licenses": ["AGPL-V3"],
    "vcs_url": "https://github.com/keriew/julius.git"
  },

  "version": {
    "name": "$VERSION",
    "released": "$(date +'%Y-%m-%d')",
    "desc": "Automated macOS build for Travis-CI job: $TRAVIS_JOB_WEB_URL"
  },

  "files": [
    {
      "includePattern": "${build_dir}/juliusgc.dmg",
      "uploadPattern": "juliusgc-$VERSION-mac.dmg",
      "listInDownloads": true
    }
  ],

  "publish": true
}
EOF
elif [ "$DEPLOY" = "vita" ]
then
cat > "bintray.json" <<EOF
{
  "package": {
    "subject": "keriew",
    "repo": "$REPO",
    "name": "vita$NAME_SUFFIX",
    "licenses": ["AGPL-V3"],
    "vcs_url": "https://github.com/keriew/julius.git"
  },

  "version": {
    "name": "$VERSION",
    "released": "$(date +'%Y-%m-%d')",
    "desc": "Automated Vita build for Travis-CI job: $TRAVIS_JOB_WEB_URL"
  },

  "files": [
    {
      "includePattern": "${build_dir}/juliusgc.vpk",
      "uploadPattern": "juliusgc-$VERSION-vita.vpk",
      "listInDownloads": true
    }
  ],

  "publish": true
}
EOF
elif [ "$DEPLOY" = "switch" ]
then
cat > "bintray.json" <<EOF
{
  "package": {
    "subject": "keriew",
    "repo": "$REPO",
    "name": "switch$NAME_SUFFIX",
    "licenses": ["AGPL-V3"],
    "vcs_url": "https://github.com/keriew/julius.git"
  },

  "version": {
    "name": "$VERSION",
    "released": "$(date +'%Y-%m-%d')",
    "desc": "Automated Switch build for Travis-CI job: $TRAVIS_JOB_WEB_URL"
  },

  "files": [
    {
      "includePattern": "${build_dir}/juliusgc_switch.zip",
      "uploadPattern": "juliusgc-$VERSION-switch.zip",
      "listInDownloads": true
    }
  ],

  "publish": true
}
EOF
fi
