### HueCommand
A program to control Hue Lights via the command line

Created as a project for myself to practice using C and curl.


## Building

To build the program perform the following from the root directory
```
mkdir build
cd build
cmake ..
make
```
And the program will be built as hueCommand

## Setup

Before the program can be used we need to obtain an auth token from the hue bridge.
You will need the ip of the hue bridge and have physical access to it for the next step
Run the following command
```
./hueCommand init-config
```
To begin the setup, enter the ip of the hue brige when requested then press the button on the bridge.
Once this has been done setup is complete!



## Commands
Commands have the following layout
hueCommand <command> <sub-command> [flags]
Below are the commands and their subcommands

# init-config
Performs the inital configuration
See [Setup](##Setup)

# l (lights)
All commands relating to lights are under the command _l_
Example usage
```
hueCommand l <sub-command> [flags]
```
Not providing a sub-command will have the help for the light be printed

There are currently two subcommands
 * l   (Prints a list of lights and their state) 
 * s   (Sets the state of a light)

When using the s sub-command a target needs to be provided with the -t flag so that the targeted light can be set

## Flags
 * -t   (Sets the target of a command)
 * -b   (Sets the targets brightness)
 * on   (Turns the target on)
 * off   (Turns the target off)
