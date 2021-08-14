#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_HISTORY_ENTRIES 32

/* Our history memory management structures. */
int numHistoryEntries = 0;
char* historyEntries[MAX_HISTORY_ENTRIES] = { NULL };

/* Prototypes. */
void HistoryCleanup(void);
void signalHandler(int);
void AddHistoryEntry(char*);

int main(void)
{
	struct sigaction sa;
	char* line, * p;

	/* Let us treat our own signals! */
	rl_catch_signals = 0;

	/* TIP: Disablea autocompletion binding TAB key */
	rl_bind_key('\t', rl_abort);

	/* History will keep the last 32 lines. */
	stifle_history(MAX_HISTORY_ENTRIES);

	/* Setup our signal handler.
	   This method is preferable than use signal() */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signalHandler;
	sigaction(SIGINT, &sa, NULL);

	/* Read lines loop! */
	for (;;)
	{
		/* if a line is typed... */
		if ((line = readline("> ")) != NULL)
		{
			/* Get rid of the final '\n' char! */
			p = strchr(line, '\n');

			if (p != NULL)
				*p = '\0';

			/* QUIT or quit exits */
			if (strcasecmp(line, "quit") == 0)
				break;

			AddHistoryEntry(line);
			printf("Line: \"%s\".\n", line);
		}
	}

	/* Get rid of the last line! */
	free(line);

	/* Get rid of all lines on history. */
	HistoryCleanup();

	return 0;
}

/* Clean up all history entries */
void HistoryCleanup(void)
{
	int i;

	for (i = 0; i < numHistoryEntries; i++)
		free(historyEntries[i]);
}

/* Handles SIGINT */
void signalHandler(int signal)
{
	switch (signal)
	{
	case SIGINT:
		/* Put readline in a clean state after Ctrl+C. */
		rl_free_line_state();
		rl_cleanup_after_signal();

		/* HACK: Shows ^C and skip to next line! */
		puts("^C");
		break;
	}
}

/* This is necessary since readline allocate a buffer
   and stifled history didn't free older lines. */
void AddHistoryEntry(char* line)
{
	int index;

	index = numHistoryEntries + 1;

	/* Frees the first entry if
	   buffer is full. */
	if (index == MAX_HISTORY_ENTRIES)
	{
		free(historyEntries[0]);

		/* And move the entire buffer
		   1 position back! */
		memcpy(historyEntries,
			&historyEntries[1],
			sizeof(char*) * numHistoryEntries);
	}

	/* Store the line */
	historyEntries[numHistoryEntries] = line;

	if (index < MAX_HISTORY_ENTRIES)
		numHistoryEntries++;

	/* put the line in the readline history! */
	add_history(line);
}