# shell_emulator
small bash - like terminal.  Suppports (3) built-in commands that fork processes as blocking or background commands.

Built-In commands:
exit - exits the program
status - checks exit status of last command
cd - as expected

supports background and foreground processes using the '&' symbol.

the other commands are parsed and passed back to the shell, if valid they will run as expected, if invalid
will return error.


best if using bash terminal.

to compile the program, I've included a bash script.
run "bash compile" 
then ./smallsh to run

if for any reason the bash script does not work, please copy 
the gcc from 'compile' and simply run that in the terminal. 
