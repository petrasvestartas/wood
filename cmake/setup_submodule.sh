#!/usr/bin/env bash
set -e
if [ ! -d ".git" ]; then
  git init
fi
if [ ! -d "ext/session_cpp/.git" ]; then
  mkdir -p ext
  git submodule add https://github.com/petrasvestartas/session_cpp.git ext/session_cpp
fi
git submodule update --init --remote --merge ext/session_cpp
echo "session_cpp submodule up to date."
