#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define HHUBUFFERSIZE 4096
#define MAX_INPUT 256

char *strdup(const char *s);

int interpret (char* input);
int do_interpret(char args[][MAX_INPUT], char* buffer);
void cd (char args[][MAX_INPUT]);
void ls (char args[][MAX_INPUT], char* buffer);
void date (char args[][MAX_INPUT], char* buffer);
void echo (char args[][MAX_INPUT], char* buffer);
void grep (char args[][MAX_INPUT], char* buffer, int piping);
void history (char args[][MAX_INPUT], char* buffer);
void split_string (char* string, char splitted[][MAX_INPUT], char* delimiter);
int search_file(char *regex, FILE *f, char* buffer);
void add_to_history(char* entry);
void read_history(void);
void write_history(char* dir);

typedef struct {
	int number;
	char* entry;
} history_entry;

history_entry* history_array;
int latest_history = 0;
int max_history = 0;

int
main()
{
	// Needed for saving history in the starting directory
	char starting_dir[MAX_INPUT];
	getcwd(starting_dir, MAX_INPUT);

	read_history();
	while (1) {
		char dir[MAX_INPUT];
		printf("%s $ ", getcwd(dir, MAX_INPUT));

		char input[MAX_INPUT];
		fgets(input, sizeof(input), stdin);
		add_to_history(input);
		int returncode = interpret(input);

		if (returncode == 1) {
			write_history(starting_dir);

			// Clean up
			int i;
			for (i=0; i<latest_history; i++)
				free(history_array[i].entry);
			return 0;
		}
	}
}

/*
 * Returns 0 for nothing interesting, 1 to exit
 */
int
interpret (char* input)
{
	char *newline, *tab, *pipe;
	int i, result, piping;
	char buffer[HHUBUFFERSIZE];

	// Remove the newline from input
	newline = strchr(input, '\n');
	*newline = '\0';
	
	// Clean input
	buffer[0] = '\0';

	// Tabs need be spaces
	while((tab = strchr(input, '\t')) != NULL)
		*tab = ' ';

	// Check if we are piping
	pipe = strchr(input, '|');
	piping = pipe != NULL;

	if (piping) {
		char parts[2][MAX_INPUT];
		char args[2][128][MAX_INPUT];

		// Clear args
		for (i=0; i<128; i++) {
			args[0][i][0] = '\0';
			args[1][i][0] = '\0';
		}

		// Copy everything 2 chars after the pipe (to skip both pipe and the space) into the second part
		strcpy(parts[1], pipe+2*sizeof(char));
		// Make pipe zero now and copy the modified input into the first part
		*pipe = '\0';
		strcpy(parts[0], input);

		split_string(parts[0], args[0], " ");
		split_string(parts[1], args[1], " ");

		// cd somewhere | grep blarp does not work – Neither does anything other then grep after the pipe
		if ((strcmp(args[0][0], "cd") == 0)) {
			puts("invalid arguments");
			return 0;
		}
		if  (strcmp(args[1][0], "grep") != 0) {
			puts("command not found");
			return 0;
		}

		result = do_interpret(args[0], buffer);

		// If the buffer is empty we already printed an error message -> return
		if (buffer[0] == '\0')
			return 0;

		grep(args[1], buffer, piping);
	} else {
		char args[128][MAX_INPUT];

		// Clear args
		for (i=0; i<128; i++)
			args[i][0] = '\0';

		split_string(input, args, " ");

		result = do_interpret(args, buffer);
	}

	printf(buffer);
	return result;
}

int
do_interpret(char args[][MAX_INPUT], char* buffer) {
	if (strcmp(args[0], "cd") == 0)
		cd(args);
	else if (strcmp(args[0], "ls") == 0)
		ls(args, buffer);
	else if (strcmp(args[0], "date") == 0)
		date(args, buffer);
	else if (strcmp(args[0], "grep") == 0)
		// This is always called as the first part of the pipe, second one is always grep
		grep(args, buffer, 0);
	else if (strcmp(args[0], "echo") == 0)
		echo(args, buffer);
	else if (strcmp(args[0], "history") == 0)
		history(args, buffer);
	else if (strcmp(args[0], "exit") == 0)
		return 1;
	else if (strcmp(args[0], "") == 0)
		return 0;
	else
		puts("command not found");

	return 0;
}


void
cd (char args[][MAX_INPUT])
{
	// Check whether we got some arguments we shouldn't get or didn't get any arguments at all
	if (args[2][0] != 0 || args[1][0] == 0) {
		puts("invalid arguments");
		return;
	}

	char* path = args[1];
	int returncode;
	returncode = chdir(path);
	if (returncode == -1) {
		int errcode = errno;
		if (errcode == ENOENT)
			puts("no such file or directory");
	}
}

void
ls (char args[][MAX_INPUT], char* buffer)
{
	// Check whether we got some arguments we shouldn't get
	if (args[1][0] != 0) {
		puts("invalid arguments");
		return;
	}
	
	DIR *dir;
	struct dirent *ent;
	char working_directory[MAX_INPUT];
	// Open working directory
	if ((dir = opendir(getcwd(working_directory, MAX_INPUT))) != NULL) {
		// Print all the files in the working directory
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] != '.') {
				strcat(buffer, ent->d_name);
				strcat(buffer, "\n");
			}
		}
		closedir (dir);
	}
}

void
date (char args[][MAX_INPUT], char* buffer)
{
	// Check whether we got some arguments we shouldn't get
	if (args[1][0] != 0) {
		puts("invalid arguments");
		return;
	}
	
	time_t t = time(NULL);
	gmtime(&t);
	struct tm *currenttm = localtime(&t);
	if (currenttm == NULL)
		buffer = "Error in date() function, tmptr is NULL";
	else
		strcpy(buffer, asctime(currenttm));
}

void
echo (char args[][MAX_INPUT], char* buffer)
{
	int i = 1;
	// If args[i][0] == 0 there's no more text to be echoed
	while (args[i][0] != '\0') {
		// After the first argument we need a space before every argument
		if (i > 1)
			strcat(buffer, " ");

		strcat(buffer, args[i]);
		i++;
	}

	// We expect buffer now to have a newline, so we need to add it manually, if there's none
	if (strchr(buffer, '\n') == NULL)
		strcat(buffer, "\n");
}

void
grep (char args[][MAX_INPUT], char* buffer, int piping)
{
	/* Check whether we got some arguments we shouldn't get or didn't get any arguments at all
	 * or got a third arg, but are piping, or did get a third arg, but aren't piping
	 */
	if (args[3][0] != 0 || args[1][0] == 0 || (args[2][0] != 0 && piping) || (args[2][0] == 0 && !piping)) {
		puts("invalid arguments");
		// Reset the buffer, for the case we are piping, and are supposed to do something with it, but can't do so, because of invalid args
		buffer[0] = '\0';
		return;
	}
	
	if (piping) {
		int i;

		// We're piping, thus buffer has the output of whatever happened before the pipe – we need to split that at newlines now
		char splitted[256][256];
		// Initialize
		for (i=0; i<256; i++)
			splitted[i][0] = '\0';

		split_string(buffer, splitted, "\n");

		// We don't need the buffer now, so we reset it
		buffer[0] = '\0';

		i = 0;
		// While there's still lines to scan
		while (splitted[i][0] != '\0') {
			// If we got a hit append it to buffer with a \n
			if (strstr(splitted[i], args[1]) != NULL) {
				strcat(buffer, splitted[i]);
				strcat(buffer, "\n");
			}
			i++;
		}

		// Append a newline if needed
		if (buffer[0] != '\0' && strchr(buffer, '\n') == NULL)
			strcat(buffer, "\n");
	} else {
		FILE *f;

		f = fopen(args[2], "r");
		if (f == NULL) {
			puts("no such file or directory");
			return;
		}
		search_file(args[1], f, buffer);
		fclose(f);
	}
}

void
history (char args[][MAX_INPUT], char* buffer)
{
	// Check whether we got some arguments we shouldn't get
	if (args[2][0] != 0 || (args[1][0] == '-' && args[1][1] != 'c')) {
		puts("invalid arguments");
		return;
	}

	int i;

	if (args[1][0] == 0)
		// Start at entry 0
		i = 0;
	else if (strcmp(args[1], "-c") == 0) {
		// free each entry individually to avoid leaks
		for (i=0; i<latest_history; i++)
			free(history_array[i].entry);

		latest_history = 0;
		// Don't print any history
		return;
	} else {
		int n = atoi(args[1]);

		if (n <= 0) {
			puts("invalid arguments");
			return;
		} else if (n > latest_history)
			n = latest_history;

		i = latest_history-n;
	}

	while (i<latest_history) {
		char formatted[MAX_INPUT];
		sprintf(formatted, "%d %s", history_array[i].number, history_array[i].entry);
		strcat(buffer, formatted);
		i++;
	}
}

// Splits a string at given delimiter
void
split_string (char* string, char splitted[][MAX_INPUT], char* delimiter)
{
	char *token;

	token = strtok(string, delimiter);

	// If we didn't even find one delimiter quit now
	if (token == NULL)
		return;

	strcpy(splitted[0], token);

	int i = 1;
	while ((token = strtok(NULL, delimiter)) != NULL) {
		strcpy(splitted[i], token);
		i++;
	}
}

// Search a file for a regex (or rather string)
int
search_file(char *regex, FILE *f, char* buffer)
{
	char buf[BUFSIZ];

	while (fgets(buf, sizeof(buf), f) != NULL) {
		if (strstr(buf, regex) != NULL)
			strcat(buffer, buf);
	}
	return 0;
}

void
add_to_history(char* line)
{
	// Check if we need to realloc
	if (latest_history == max_history) {
		// Start with 10 entries, or add 10 entries
		if (max_history == 0)
			max_history = 10;
		else
			max_history += 10;

		// Temporarily allocate a new history, to not lose the old one in case of OOM 
		void *tmp = realloc(history_array, (max_history * sizeof(history_entry)));

		if (tmp == NULL) {
			puts("ERROR: Could not allocate memory!");
			return;
		}

		history_array = (history_entry*)tmp;
	}

	// Malloc a new string, because line will change next time we type something
	char* tmpline = strdup(line);
	history_entry entry = {latest_history, tmpline};
	history_array[latest_history] = entry;
	latest_history++;
}

void
read_history ()
{
	FILE *f;
	char buf[BUFSIZ];

	f = fopen(".hhush.histfile", "r");

	// Check if a history file is available
	if (f != NULL) {
		while (fgets(buf, sizeof(buf), f) != NULL)
			add_to_history(buf);

		fclose(f);
	}
}

void
write_history (char* dir)
{
	int i;
	FILE* f;
	char file[MAX_INPUT];

	// Compute the path we should save in
	sprintf(file, "%s/.hhush.histfile", dir);
	
	f = fopen(file, "w");

	for (i=0; i<latest_history; i++)
		fputs(history_array[i].entry, f);

	fclose(f);
}