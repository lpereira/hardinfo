# Project

https://github.com/hwspeedy/hardinfo


## Issues 

Please write hardinfo version, distribution and distribution verision
architecture and explain the issues as good as possible so it can be
reproduced, thanx.

These tools can be used to provide the above information:
uname -a
cat /etc/os-release
hardinfo -v


## Pull requests

Pull requests should contain related commits. Each commit should have a
descriptive title, and clearly describe the changes in the commit message.
Please make small commits that are easier to review.

If this is your first contribution, feel free to add your name to
shell/callbacks.c - about() as a commit included in your pull request.


## Coding

Ancient code used a mixture of tab and space for indentation, but newer
prefers four (4) spaces for each level of indentation.


# Translators

If possible, run `bash updatepo.sh` in the po/ directory before begining.
This will regenerate hardinfo.pot and update all the .po files.
Nothing is lost if this isn't done, but it prevents wasting time translating
strings that have changed, or are no longer used.

DO NOT use `make pot`!
