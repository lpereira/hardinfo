#!/bin/bash
echo "Changing to latest git release..."
git pull
git checkout $(curl -s https://hardinfo2.org/github/?latest_git_release)
