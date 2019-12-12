OX_chess
=========

## Features
1. allow multiple client connecting to server at the same time
2. clients can choose a online player to fight with and ask for his or her agreement
3. two paired players can take turns to play OX-chess until one player win.

## Requirements
- GNU-g++ compiler

## Build
```
$ git clone https://github.com/RAchange/OX_chess
$ cd OX_chess
$ make
```

## Implements

You can see the argument by pass the `h` flag 
```
$ ./OX_server -h
OX_chess server

NAME
	OX_server - a OX_chess server for mult-players

SYNOPSIS
	./OX_server [options]

OPTIONS
	-a ip address
		 The ip address you want to bind.
	-p port number
		 The port number you want to bind
```

### server
```
$ ./OX_server -p 1000
```
### client
```
$ ./OX_server -p 1000
```
#### commands
1. To list all the online players
> list
2. To find someone to pair with
> pair
> <player id>
3. To select a position to occupy
> move(<row>,<column>)
4. To quit the game
> quit