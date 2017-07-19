Index:

* [Coding guidelines] (#coding-guidelines)
* [Develop info] (#develop-info)

# Coding guidelines

Pleae coordinate effors in github pull and issues, 
but take in consideration that time of each coder/contributor are limited to review/see your issue or pull.

## Pull request

**Each pull request must be in a behaviour specified functionality**, 
this to avoid conflicts and must avoid multiple commits corrections.. each commit must have a maximun of 8 commits
and the changes must be around 2000 lines of code, please be consistent and short in changes.. 

**For large pull's like new features** must firts discuss with main ower of the proyect and coders, 
to coordinate free time and made revision of code.

As example: if you plan to featured a translation (update contents) and also added new subsection module, dont 
put all the code in one commit, must be firts the functional code the new subsection, and then (in two parts/pulls) 
the translation updates. Why? well if code of the firs pull are not acepted, the code of the translation still are valid.
Of course if the pull of the new subsection module are acepted the pull of the translation are out of date.. 
in that case can be made a new pull with translation update or update the pull translation if are still not acepted/merged.

## Coding style

* identations: Currently the code use as default a TAB for identations. 
* line size: Consider Screen Size Limits as default 100 to 120 chars max as overall
* Brace Placement: start brace in same line, end brace in new line
* Header File Guards: Include files should protect against multiple inclusion.

Please take care of against multiple inclusion through the use of macros that "guard" the files.
This due please you must take in consideration backguard compatibility with older versions of dependences.

# Develop info

## Overall modules plugins info for Developmen

Modules used a general `module_call_method_param` where give the function as string and then use an hast table.. 

**Modules behaviour**: Each module (computer, devices, etc.) communicate back to the shell by passing back strings formatted in a certain way. 
And that's the way a module can request information from another module; for instance, the computer module can obtain 
information from the devices module to build the summary information.

**Module writing:** Modules can also obtain information from the same module by calling this function from the C code as well; 
that's how the benchmark module gets which CPU it's running on. The idea is that people could write their own 
modules

## About old server-client missing

The (now missing) server protocol for synchronizing stuff indeed uses `XML-RPC`. Code was get missing of server side, 
client code are still present in `hardinfo` but are not currently activate by default when will compile.

There are just two function, **one to get server data/info** and **other to upload data/info**. 

The one to upload/retrieve data is used by many other functions; grep for calls to `sync_manager_add_entry()`. 
Some things have their own format (like pci.ids file for lspci), but most (including vendor list, CPU flag list, 
benchmark data) are in the GLib GKeyFile format, which is essentially a format inspired by Guindows .ini files.
