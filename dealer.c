/* ----------------------------------------------------------- */
/* NAME : John Mortimroe                     User ID: jgmortim */
/* DUE DATE : 02/24/2019                                       */
/* PROGRAM ASSIGNMENT 2                                        */
/* FILE NAME : dealer.c                                        */
/* PROGRAM PURPOSE :                                           */
/*    This program is a dealer that creates the specified      */
/*    number of player process for a card game.                */
/*                                                             */
/*    Tested with Valgrind 2/23/19: no memory leaks or errors. */
/* ----------------------------------------------------------- */
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

void shuffleDeck();
int draw();
void dealStd();
void dealTie(int *playerList, int playerCount);
int elim(int *plres);
int tieBreak(int *players, int count);
int createPlayers();
void usageError();
void pipeError();
void forkError();
void cleanup();

int deck[52];		/* Deck of cards. */
int deckIndex=0;	/* Position in deck. */
int numPlayers;		/* Number of players. */
char *output;		/* Output String. */
int *pids;		/* Array of pids. */
int **pipes;		/* Array of pipes. */
int *plres;		/* Array of player responses. */

/* ----------------------------------------------------------- */
/* FUNCTION : main                                             */
/*    The main method for the program.                         */
/* PARAMETER USAGE :                                           */
/*    argc {int} - number of argument.                         */
/*    argv {char**} - list of arguments.                       */
/* FUNCTIONS CALLED :                                          */
/*    atoi(), cleanup(), createPlayers(), dealStd(), elim(),   */
/*    forkError(), kill(), malloc(), pipe(), pipeError(),      */
/*    read(), shuffleDeck(), sizeof(), sprintf(), strlen(),    */
/*    usageError(), write()                                    */
/* ----------------------------------------------------------- */
int main(int argc, char** argv){ 
	int i=0;
	int numPinGame;
	int elimID=0;
	char *oReport=" reports a total of ";
	char *oKnock=" is knocked out!\n";
	char *oEnd="Ending the game.\n";
	/* If incorrect number of arguments, print usage and exit. */
	if (argc != 2) {
		usageError();
		return 1;
    	}
	/* Get number of players*/
	numPlayers =atoi(argv[1]);
	/* If incorrect parameter, print usage and exit. */
	if (numPlayers<1) {
		usageError();
		return 1;
    	}
	/* Number of players in game starts at number of players. */
	numPinGame = numPlayers;
	/* Allocate space for pipes, player responses, player IDs, and output Strings. */
	pipes=malloc(sizeof(int*)*numPlayers*2);
	for (i = 0; i < 2*numPlayers; ++i) {
		pipes[i] = malloc (sizeof (int) * 2);
	}
	plres=malloc(sizeof(int)*numPlayers);
	pids=malloc(sizeof(int)*numPlayers);
	output=malloc(sizeof(char)*100);
	
	/* Create Pipes, plus error checking. */
	for (i=0; i < 2*numPlayers; i++) {
		if (pipe(pipes[i]) != 0) {
			pipeError();
			cleanup();
			return 1;
		}
	}
	/* Shuffle the deck. */
	shuffleDeck();
	/* Create players and error checking.*/
	if (createPlayers()==-1) {
		forkError();
		cleanup();
		return 1;
	};
	/* Repeat until there is only one player in the game */
	while(numPinGame > 1) {
		/* Deal cards to players. */
		dealStd();
		/* Get Player Responses and print them. */
		for (i=0; i < numPlayers; i++) {
			if (pids[i]!=-1) {
				read(pipes[(i*2)+1][0], &plres[i], sizeof(int));
				sprintf(output, "Player %d%s%d.\n", i+1, oReport, plres[i]);
				write(1, output, strlen(output));
			}
		}
		/* Find the player to eliminate. */
		elimID = elim(plres);
		/* Do not term a process twice. */
		if (pids[elimID] != -1) {
			/* players responce will be ignored moving forward. */
			plres[elimID]=INT_MAX;
			/* Term player, print termination.*/
			kill(pids[elimID], 15);
			sprintf(output, "Player %d%s", elimID+1, oKnock);
			write(1, output, strlen(output));
			/* Cards will not be dealt to player again. */
			pids[elimID]=-1;
			/* Decrease the number of players in game. */
			numPinGame--;
		}
		
	}
	/* Print the winner and term the last player process. */
	for (i=0; i<numPlayers; i++) {
		if (pids[i]!=-1) {
			sprintf(output, "Player %d Wins!\n", i+1);
			write(1, output, strlen(output));
			kill(pids[i], 15);
		}
	}
	/* Free allocated memory, print game over, and exit. */
	cleanup();
	write(1, oEnd, strlen(oEnd));
	return 0;
}

/* ----------------------------------------------------------- */
/* FUNCTION : dealStd                                          */
/*    Deal cards to the players under normal circumstances.    */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    draw(), sizeof(), sprintf(), strlen(), write()           */
/* ----------------------------------------------------------- */
void dealStd() {
	int cards[4];
	int i;
	/* Deal 4 cards to each player still in the game. */
	for (i=0; i < numPlayers; i++) {
		if (pids[i]!=-1) {
			cards[0]=draw();
			cards[1]=draw();
			cards[2]=draw();
			cards[3]=draw();
			sprintf(output, "Dealing [%2d, %2d, %2d, %2d] to player %d.\n", cards[0], cards[1],  cards[2], cards[3], i+1);
			write(1, output, strlen(output));
			write(pipes[i*2][1], cards, sizeof(int)*4);
		}
	}
}

/* ----------------------------------------------------------- */
/* FUNCTION : dealTie                                          */
/*    Deal cards to the players to break a tie.                */
/* PARAMETER USAGE :                                           */
/*    playerList {int*} - the list of players that tied.       */
/*    playerCount {int} - the number of players that tied.     */
/* FUNCTIONS CALLED :                                          */
/*    draw(), sizeof(), sprintf(), strlen(), write()           */
/* ----------------------------------------------------------- */
void dealTie(int *playerList, int playerCount) {
	int cards[4];
	int i;
	/* Deal 4 cards to each tying player. */
	for (i=0; i < playerCount; i++) {
		cards[0]=draw();
		cards[1]=draw();
		cards[2]=draw();
		cards[3]=draw();
		sprintf(output, "Dealing [%2d, %2d, %2d, %2d] to player %d.\n", cards[0], cards[1],  cards[2], cards[3], playerList[i]+1);
		write(1, output, strlen(output));
		write(pipes[playerList[i]*2][1], cards, sizeof(int)*4);
	}
}

/* ----------------------------------------------------------- */
/* FUNCTION : elim                                             */
/*    Selects the player to be eliminated. Breaks ties when    */
/*    needed. returns the number of the player to be           */
/*    eliminated.                                              */
/* PARAMETER USAGE :                                           */
/*    plres {int*} - a list of the player responses (totals).  */
/* FUNCTIONS CALLED :                                          */
/*    free(), malloc(), sizeof(), tieBreak()                   */
/* ----------------------------------------------------------- */
int elim(int *plres) {
	int i, elimID;
	int minResult=INT_MAX;
	int tyingCount=0;
	int *minPlayers=malloc(sizeof(int)*numPlayers); 
	/* minPlayers is allocated on every call because this method is used */
        /* recursively when a tie occures.                                   */

	/* Find the player(s) with the minimum score */
	for (i=0; i<numPlayers; i++) {
		if (plres[i] < minResult) {
			minResult = plres[i];
			minPlayers[0] = i;
			tyingCount = 1;
		} else if (plres[i] == minResult) {
			minPlayers[tyingCount] = i;
			tyingCount++;
		}
	}
	/* Break tie if necessary. */
	if (tyingCount > 1) {
		elimID = tieBreak(minPlayers, tyingCount);
	} else {
		elimID = minPlayers[0];
	}
	/* Free up allocated memory and return the loser. */
	free(minPlayers);
	return elimID;
}

/* ----------------------------------------------------------- */
/* FUNCTION : tieBreak                                         */
/*    Breaks ties and returns the lossers number.              */
/* PARAMETER USAGE :                                           */
/*    players {int*} - a list of the player to tie break.      */
/*    count {int} - the number of players to tie break.        */
/* FUNCTIONS CALLED :                                          */
/*    dealTie(), elim(), read(), sizeof(), sprintf(),          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
int tieBreak(int *players, int count) {
	char *oReport=" reports a total of ";
	int i;
	/* Print list of tying players. */
	sprintf(output, "Tie between players: ");
	write(1, output, strlen(output));
	for (i=0; i < count-1; i++) {
		if (count > 2) {
			sprintf(output, "%d, ", players[i]+1);
		} else {
			sprintf(output, "%d ", players[i]+1);
		}
		write(1, output, strlen(output));
	}
	sprintf(output, "and %d.\nDealing more cards.\n", players[i]+1);
	write(1, output, strlen(output));
	/* Deal new cards to players. */
	dealTie(players, count);
	/* Default all responses to max int. This prevents the non-tying */
	/* players from being considered when elim() is called.          */
	for (i=0; i < numPlayers; i++) {
		plres[i]=INT_MAX;
	}
	/* Get the responses (totals) from the players. */
	for (i=0; i < count; i++) {
		read(pipes[(players[i]*2)+1][0], &plres[players[i]], sizeof(int));
		sprintf(output, "Player %d%s%d.\n", players[i]+1, oReport, plres[players[i]]);
		write(1, output, strlen(output));
	}
	/* Send responses to elim to get losser. */
	return elim(plres);
}

/* ----------------------------------------------------------- */
/* FUNCTION : draw                                             */
/*    Draw a card from the deck.                               */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    shuffleDeck()                                            */
/* ----------------------------------------------------------- */
int draw() {
	int card;
	card = deck[deckIndex];
	deckIndex++;
	if (deckIndex >= 52) {
		shuffleDeck();
		deckIndex=0;
	}
	return card;
}

/* ----------------------------------------------------------- */
/* FUNCTION : shuffleDeck                                      */
/*    Shuffle a new deck.                                      */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    rand(), srand(), time()                                  */
/* ----------------------------------------------------------- */
void shuffleDeck() {
	int i, j, temp;
	srand(time(0));
	/* Fill the deck with cards. */
	for (i=0; i<52; i++) {
		deck[i]=(i%13)+1;
	}
	/* Shuffle the Cards. */
	for (i=0; i<52; i++) { 
        	j = i+(rand()%(52-i)); 
		temp = deck[i];
		deck[i]=deck[j];
		deck[j]=temp;
	}
}

/* ----------------------------------------------------------- */
/* FUNCTION : createPlayers                                    */
/*    Creates all of the player process.                       */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    close(), dup2(), execv(), fork()                         */
/* ----------------------------------------------------------- */
int createPlayers() {
	int i;
	char **arg={NULL};
	/* Create player processes and set-up pipes. Plus error checking. */
	for (i=0; i < numPlayers; i++) {
		pids[i]=fork();
		if (pids[i] == -1) {
			return -1;
		} else if (pids[i] == 0) {
			if (dup2(pipes[i*2][0], 0) == -1 || dup2(pipes[i*2+1][1], 1) == -1) {
				return -1;
			}
			close(pipes[i*2][0]);
			close(pipes[i*2][1]);
			close(pipes[i*2+1][0]);
			close(pipes[i*2+1][1]);
			if (execv("player", arg) == -1) {
				return -1;
			}
		} else {
			close(pipes[i*2][0]);
			close(pipes[i*2+1][1]);
		}
	}
	return 0;
}

/* ----------------------------------------------------------- */
/* FUNCTION : cleanup                                          */
/*    frees allocated memory when an error occures.            */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    free()                                                   */
/* ----------------------------------------------------------- */
void cleanup() {
	int i;
	for (i = 0; i < 2*numPlayers; ++i) {
		free(pipes[i]);
	}
	free(pipes);
	free(plres);
	free(pids);
	free(output);
}

/* ----------------------------------------------------------- */
/* FUNCTION : forkError                                        */
/*    Called when an error occures with fork() or execv().     */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void forkError() {
	char *error1="Error encountered while trying to create players processes.\n";
	char *error2="Execution terminated.\n";
	write(1, error1, strlen(error1));
	write(1, error2, strlen(error2));
}

/* ----------------------------------------------------------- */
/* FUNCTION : pipeError                                        */
/*    Called when an error occures with pipe().                */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void pipeError() {
	char *error1="Error encountered while trying to create pipes.\n";
	char *error2="Execution terminated.\n";
	write(1, error1, strlen(error1));
	write(1, error2, strlen(error2));
}

/* ----------------------------------------------------------- */
/* FUNCTION : usageError                                       */
/*    Called when an program in not invoked properly.          */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void usageError() {
	char *error1="Usage :\ndealer <number of players> \n";
	char *error2="Where <number of players> is an integer number which specify the number of players in the game.\n";
	char *error3="This number can be no less than 1.\n";
	write(1, error1, strlen(error1));
	write(1, error2, strlen(error2));
	write(1, error3, strlen(error3));
}

