Overall info for contribution on source:

* [Coding contribution] (#coding-contribution)
* [Translations] (#translations)

# Coding contribution

All the collaboration work must be using https://github.com/lpereira/hardinfo issues and pull request.
So generally, clone the repository with git, and use branches, before make a pull request.

## Pull requests

Please **each and all the pull request must made in related scope**, 
this to avoid conflicts, and must avoid massive commits corrections.. 
large pulls will take more time from owner to review and commit!

Pease **each pull request must have descriptive related titles** and 
clarelly description in the pull request, about changes in each commits, 
and must mantain those amount of commits in blocks and not to be massive.

## Coding

Most ancient code use a mixture  of TAB and SPACES, but newer must 
prefered and priority its to use 4 SPACES!

# Translations

We using po files, its recommended before beginning, 
try to run `bash updatepo.sh` in the po/ directory to sync files.

This will regenerate hardinfo.pot and update all the .po files. Dont worry, 
NOTHING IS LOST if this isn't done, but it prevents wasting time translating
strings that have changed, or are no longer used.

