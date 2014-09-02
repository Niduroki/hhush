#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define HHUBUFFERSIZE 4096

char *get_current_dir_name(void);
char *strdup(const char *s);

int interpret (char* input);
int do_interpret(char args[][256], char* buffer);
void cd (char args[][256]);
void ls (char args[][256], char* buffer);
void date (char args[][256], char* buffer);
void echo (char args[][256], char* buffer);
void grep (char args[][256], char* buffer, int piping);
void history (char args[][256], char* buffer);
void removeNL(char* string);
void split_string (char* string, char splitted[][256], char* delimiter);
void tab_to_space (char* string);
int search_file(char *regex, FILE *f, char* buffer);
int search_text(char *regex, char *text);
void add_to_history(char* entry);

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
	int run = 1;
	// TODO load history here
	// If history loaded
	if (1) {
		// TODO set latest_history to something
	}
	while (run) {
		char *dir = get_current_dir_name();
		printf("%s $ ", dir);
		free(dir);
		char input[256];
		fgets(input, sizeof(input), stdin);
		add_to_history(input);
		int returncode = interpret(input);
		if (returncode == 1) {
			// We're supposed to exit, save the history
			// TODO
			int i;
			for (i=0; i<latest_history; i++) {
				free(history_array[i].entry);
			}
			free(history_array);
			return 0;
		}
	}
	return 0;
}

/*
 * Returns 0 for nothing interesting, -1 on error, 1 to exit
 */
int
interpret (char* input)
{
	int i, result;
	char buffer[HHUBUFFERSIZE];
	
	// Clean input
	buffer[0] = '\0';

	// Tabs need be spaces
	tab_to_space(input);

	// Check if we are piping
	char* pipe = strchr(input, '|');
	int piping = pipe != NULL;

	if (piping) {
		char firsthalf[256];
		char secondhalf[256];
		char firstargs[128][256];
		char secondargs[128][256];

		// Clear args
		for (i=0; i<128; i++) {
			firstargs[i][0] = '\0';
			secondargs[i][0] = '\0';
		}

		// Copy everything 2 chars after the pipe (to skip both pipe and the space) into secondhalf
		strcpy(secondhalf, pipe+2*sizeof(char));
		// Make pipe zero now and copy the modified input into firsthalf
		*pipe = '\0';
		strcpy(firsthalf, input);

		split_string(firsthalf, firstargs, " ");
		split_string(secondhalf, secondargs, " ");

		// Remove a stray newline
		removeNL(firstargs[0]);
		removeNL(secondargs[0]);

		// cd somewhere | grep blarp does not work – Neither does anything other then grep after the pipe
		if ((strcmp(firstargs[0], "cd") == 0)) {
			puts("invalid arguments");
			return 0;
		}
		if  (strcmp(secondargs[0], "grep") != 0) {
			puts("command not found");
			return 0;
		}

		result = do_interpret(firstargs, buffer);

		// Nothing is in the buffer, thus we already printed an error message → return
		if (buffer[0] == '\0') {
			return 0;
		}

		grep(secondargs, buffer, piping);
	} else {
		char args[128][256];

		// Clear args
		for (i=0; i<128; i++)
			args[i][0] = '\0';

		split_string(input, args, " ");

		/* For whatever reason there'a stray newline */
		removeNL(args[0]);

		result = do_interpret(args, buffer);
	}

	printf(buffer);
	return result;
}

int
do_interpret(char args[][256], char* buffer) {
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
	else {
		puts("command not found");
		return 0;
	}

	return 0;
}


void
cd (char args[][256])
{
	// Check whether we got some arguments we shouldn't get or didn't get any arguments at all
	if (args[2][0] != 0 || args[1][0] == 0) {
		puts("invalid arguments");
		return;
	}

	char* path = args[1];
	int returncode;
	removeNL(path);
	returncode = chdir(path);
	if (returncode == -1) {
		int errcode = errno;
		if (errcode == ENOENT)
			puts("no such file or directory");
	}
}

void
ls (char args[][256], char* buffer)
{
	// Check whether we got some arguments we shouldn't get
	if (args[1][0] != 0) {
		puts("invalid arguments");
		return;
	}
	
	// TODO understand this :|
	// https://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
	DIR *dir;
	struct dirent *ent;
	char* working_directory = get_current_dir_name();
	if ((dir = opendir (working_directory)) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] != '.') {
				strcat(buffer, ent->d_name);
				strcat(buffer, "\n");
			}
		}
		closedir (dir);
	}
	free(working_directory);
}

void
date (char args[][256], char* buffer)
{
	// Check whether we got some arguments we shouldn't get
	if (args[1][0] != 0) {
		puts("invalid arguments");
		return;
	}
	
	time_t t = time(NULL);
	gmtime(&t);
	struct tm *currenttm = localtime(&t);
	if (currenttm == NULL) {
		strcat(buffer, "Error in date() function, tmptr is NULL");
	} else {
		strcat(buffer, asctime(currenttm));
	}
}

void
echo (char args[][256], char* buffer)
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

	// I have to manually do a newline, if there's no text to echo
	if (args[1][0] == '\0')
		strcat(buffer, "\n");
}

void
grep (char args[][256], char* buffer, int piping)
{
	// Check whether we got some arguments we shouldn't get or didn't get any arguments at all or got a third arg, but are piping, or did get a third arg, but aren't piping
	if (args[3][0] != 0 || args[1][0] == 0 || (args[2][0] != 0 && piping) || (args[2][0] == 0 && !piping)) {
		puts("invalid arguments");
		// Reset the buffer, for the case we are piping, and are supposed to do something with it, but can't do so, because of invalid args
		buffer[0] = '\0';
		return;
	}
	
	if (piping) {
		int i;

		// Remove a sneaky newline
		removeNL(args[1]);
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
			if (search_text(args[1], splitted[i])) {
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

		// For whatever reason there's a newline
		removeNL(args[2]);

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
history (char args[][256], char* buffer)
{
	// Check whether we got some arguments we shouldn't get
	if (args[2][0] != 0 || (args[1][0] == '-' && args[1][1] != 'c')) {
		puts("invalid arguments");
		return;
	}

	// Remove cheeky newlines
	removeNL(args[1]);

	int i;

	if (args[1][0] == 0) {
		for (i=0; i<latest_history; i++) {
			char formatted[256];
			sprintf(formatted, "%d %s", history_array[i].number, history_array[i].entry);
			strcat(buffer, formatted);
		}
	} else if (strcmp(args[1], "-c") == 0) {
		// free each entry individually to avoid leaks
		for (i=0; i<latest_history; i++) {
			free(history_array[i].entry);
		}
		free(history_array);
		latest_history = 0;
	} else {
		int n = atoi(args[1]);

		if (n <= 0) {
			puts("invalid arguments");
			return;
		} else if (n > latest_history)
			n = latest_history;

		for (i=latest_history-n; i<latest_history; i++) {
			char formatted[256];
			sprintf(formatted, "%d %s", history_array[i].number, history_array[i].entry);
			strcat(buffer, formatted);
		}
		// TODO find out what number we got and print as many entries
	}
}

/**
 * Replaces a newline in a string/char array with a string terminating \0
 * Necessary, because splitting inserts a \n for whatever reason
 */
void
removeNL(char* string)
{
	int i;
	int n = strlen(string);
	for (i=0; i<n; i++) {
		if (string[i] == '\n') {
			string[i] = '\0';
			return;
		}
	}
}

// Splits a string at given delimiter
void
split_string (char* string, char splitted[][256], char* delimiter)
{
	char *token;

	token = strtok(string, delimiter);
	strcpy(splitted[0], token);

	int i = 1;
	token = strtok(NULL, delimiter);
	while (token != NULL) {
		strcpy(splitted[i], token);
		i++;
		token = strtok(NULL, delimiter);
	}
}

void
tab_to_space (char* string)
{
	char* tab;
	while((tab = strchr(string, '\t')) != NULL) {
		*tab = ' ';
	}
}

// Search a file for a regex (or rather string)
int
search_file(char *regex, FILE *f, char* buffer)
{
	//int n;
	char buf[BUFSIZ];

	while (fgets(buf, sizeof(buf), f) != NULL) {
		//n = strlen(buf);
		// Avoid printing newlines from the strings we've matched TODO
		/*if (n > 0 && buf[n-1] == '\n')
			buf[n-1] = '\0';*/
		if (search_text(regex, buf))
			strcat(buffer, buf);
	}
	return 0;
}

// Search a text for a regex (or rather string)
int
search_text(char *regex, char *text)
{
	int i = 0;
	// Text to search is empty?
	while (text[i] != 0) {
		// Does regex match the current position?
		if (strncmp(regex, &text[i], strlen(regex)) == 0)
			return 1;
		// Retry at the next character
		i++;
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

		// Things are looking good so far
		history_array = (history_entry*)tmp;
	}

	// Malloc a new string, because line will change next time we type something
	char* tmpline = strdup(line);
	history_entry entry = {latest_history, tmpline};
	history_array[latest_history] = entry;
	latest_history++;
}