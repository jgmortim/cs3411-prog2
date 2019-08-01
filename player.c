/* ----------------------------------------------------------- */
/* NAME : John Mortimroe                     User ID: jgmortim */
/* DUE DATE : 02/24/2019                                       */
/* PROGRAM ASSIGNMENT 2                                        */
/* FILE NAME : player.c                                        */
/* PROGRAM PURPOSE :                                           */
/*    This program is the player process for dealer.c          */
/* ----------------------------------------------------------- */

#include <stdlib.h>
#include <unistd.h>

/* ----------------------------------------------------------- */
/* FUNCTION : main                                             */
/*    The main method for the program.                         */
/* PARAMETER USAGE :                                           */
/*    argc {int} - number of argument.                         */
/*    argv {char**} - list of arguments.                       */
/* FUNCTIONS CALLED :                                          */
/*    exit(), read(), sizeof(), write()                        */
/* ----------------------------------------------------------- */
int main(int argc, char** argv){ 
	int cards[4]={0,0,0,0};
	int sum[1];
    
	while(1){
		read(0, cards, sizeof(int)*4);
		sum[0] = cards[0]+cards[1]+cards[2]+cards[3];
		write(1, sum, sizeof(int));
	}
	exit(0);
}
