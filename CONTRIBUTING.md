Overall info for contribution on source:

* [Coding contribution] (#coding-contribution)
* [Translations] (#translations)

# Coding contribution

All the collaboration work must be using https://github.com/lpereira/hardinfo issues and pull request.
So generally, clone the repository with git, and use branches, before make a pull request.

## Pull requests

Please maintain **each pull request must be in behavior/functionality specific**, 
this to avoid conflicts, and must avoid massive commits corrections.. large pulls will take more time from owner to review and commit!

## Coding

Most ancient code use a mixture  of TAB and SPACES, but newer must 
prefered and priority its to use 4 SPACES!

# Translations

We using po files, its recommended before beginning, 
try to run `bash updatepo.sh` in the po/ directory to sync files.

This will regenerate hardinfo.pot and update all the .po files. Dont worry, 
NOTHING IS LOST if this isn't done, but it prevents wasting time translating
strings that have changed, or are no longer used.

