Zork
====

Zork is a simple directory and file indexing program for Linux.

Usage
---------
zork *options* *arguments*  

Available options:  
&nbsp;&nbsp;&nbsp;&nbsp;-r Make the particular directory listing recursive. Optionaly give a recursion count.  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Using -r without the recursion count results in recursion to the innermost directory.  
&nbsp;&nbsp;&nbsp;&nbsp;-a *dir name* Add a directory entry and update database  
&nbsp;&nbsp;&nbsp;&nbsp;-u Update current database  
&nbsp;&nbsp;&nbsp;&nbsp;-t Tolerant search. Allow unmatched characters in search string. Optionally give a count  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;of how many mismatches to allow. If none given, a default of 2 is assumed.  
&nbsp;&nbsp;&nbsp;&nbsp;-s *search string* Search  
&nbsp;&nbsp;&nbsp;&nbsp;-p *program name* Open selected search result in external program  
&nbsp;&nbsp;&nbsp;&nbsp;-R *dir name* Remove a directory entry and update database  
&nbsp;&nbsp;&nbsp;&nbsp;-v Show currently indexed directories  
&nbsp;&nbsp;&nbsp;&nbsp;-h This help text  

<b>Note:</b> Multiple options can be used at once, but order of the options matter.
* -p should be used with -s and placed before -s.  
* -r should be used with -a and placed before -a.

