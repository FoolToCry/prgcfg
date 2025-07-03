# prgcfg
C stuff for evaluation of command line and config files with some example programs.

## directorry LIB 
Files for library which is used by example programs to evalute options from command line and config file during program invocation.  
To generate lib:
```
cd LIB 
make 
```

## directory ASCII
Two example programs which are using the above Library. The ascii movie program (src: ascii_mov.c) uses command line options and a configuration file. 
To generate amov executable:
```
cd ASCII
make 
```
example call:
```
amov -h
amov -f submarine.txt
```
...  
more description to follow  
...
