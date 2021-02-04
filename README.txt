ECE 4122 Final Project
Author: Zechuan Ding
Date: 2020/12/1
**This program needs to be compiled and run using Visual Studio IDE.**

Library used: <GL/glut.h>

Note: in this Pac-Man Game, the four ghosts use the algorithm to move as in the orginial game design, instead of all finding the shortest path to pacman. Using of this algorithm has been approved by the course instructor.
All four ghosts will set a target tile and move to a direction that is closer to the target tile.
- Red ghost sets target to pacman's tile. 
- Pink ghost sets target to four tiles in front of pacman (or four tiles to the up and four tiles to the left when pacman is facing up)
- Cyan ghost sets target to the tile that is symmetric position of red ghost about the pacman
- Orange ghost sets target to pacman when it is eight tiles away and sets the target to the left bottom corner when pacman is within eight tiles.


Game Control:
Press any key to start
Use arrow key to control movement of pacman
Press R to rotate map clockwise
Press W to rotate map counterclockwise
Press E to reset map
(Starting a new game requires closing the game window and reclick on the "Local Windows Debugger" botton)
