# GoldChase Game Adding Message Capabilities Project Iteration 3

## Game play enhancements
This version of Goldchase will include the following enhancements:
1. When a new player enters a game which has already started, the other players’ maps will
automatically update to show the new player’s position on the board.
2. When any player changes the map (such as, for example moving), then the other players’
maps will automatically update.
3. Type ‘m’ to send a message to another player. Prompt the user with a list of active players
from which one should be selected (see getPlayer() below). The user will then enter a
message (see getMessage() below) which will appear on the respective player’s window
in the form of a postNotice. Be sure to prepend the message with "Player # says:".
4. Type ‘b’ to broadcast a message to all other players. Prompt the user to enter a message.
This message will be broadcast to all other players as postNotice messages on their
respective windows. Be sure to prepend the message with "Player # says:".
5. When a player wins by leaving the game board with the gold, a "Player # won!"
message will be sent to the other players.
6. As before, the last person/process to leave the game is responsible for unlinking the shared
memory and semaphore.
7. If a process ends abnormally (e.g., SIGTERM, SIGINT, SIGHUP) be sure to unlink any
message queues belonging to the process. Also, unlink shared memory and the semaphore
if it is the last process


## Architectural considerations
Signal handling and message queues will be used to implement automatic screen updates and
chat functionality.
1. The Map class has two additional functions:
(1) unsigned int getPlayer(unsigned int PlayerMask) This member function
accepts a single unsigned integer with the G_PLR# bits of active players or’d together.
The function will display a box showing a player number for each bit toggled on. As
soon as a valid player number is typed in by the user, the box disappears and the
entered G_PLR# corresponding to the selected player is returned. Usage example:
Let’s say that yourself, player 5, and player 2 are playing the game. If you want to send
a message to one of these players, then an unsigned integer needs to be initialized to
(G_PLR4 | G_PLR1) and passed to the getPlayer function. A box with 5 and 2 will appear on the screen. Once the user types a ‘2’ or a ‘5’, either G_PLR1 or G_PLR4 will
be returned by the function.
(2) std::string getMessage() This member function prompts the user for a string of
text, which is returned. Use this function to query the player for the text that should be
sent to a different player.
2. The mapBoard structure will need to be expanded. Its original form included the following
fields:
struct mapBoard {
 unsigned int columns;
 unsigned int rows;
 unsigned char players;
 char board[0];
};
Expand this structure with five more integers–one for each of the possible five players (type
pid_t would be more correct than integers). A single array of 5 elements would work well.
If a player is not playing (or leaves the game), the respective integer should be set to zero.
If a player joins the game, the integer should be set to the process ID of that player’s
process. When you need to send a signal to the other players (such as when you call
drawMap), you can examine these 5 integers to determine the process IDs to send signals
to.
3. The best way to implement the chat functionality is to have a separate message queue for
each player. The “owner” of the queue will only read from the queue, all other players will
only write to the queue.
