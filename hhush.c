#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

char *get_current_dir_name(void);
int interpret (char* input);
void cd (char args[][256]);
void ls (char args[][256]);
void date (char args[][256]);
char* echo (char args[][256]);
void grep (char args[][256]);
void history (char* arg);
void removeNL(char* string);
int string_to_int(char* string);
int search_file(char *regex, FILE *f);
int search_text(char *regex, char *text);

int
main()
{
	int run = 1;
	while (run) {
		char *dir = get_current_dir_name();
		printf("%s $ ", dir);
		free(dir);
		char input[256];
		fgets(input, sizeof(input), stdin);
		// TODO save the history into an array/struct here
		int returncode = interpret(input);
		if (returncode == 1) {
			// We're supposed to exit, save the history
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
	char args[128][256];
	
	// Clean input
	int j;
	for (j=0; j<128; j++) {
		args[j][0] = '\0';
	}

	/* Split input */
	char *token;
	char inputcpy[256];
	strcpy(inputcpy, input);
	char *delimiter = " ";


	token = strtok(inputcpy, delimiter);
	strcpy(args[0], token);

	int i = 1;
	token = strtok(NULL, delimiter);
	while (token != NULL) {
		strcpy(args[i], token);
		i++;
		token = strtok(NULL, delimiter);
	}
	/* Done splitting */
	/* For whatever reason there'a stray newline though */
	removeNL(args[0]);

	if (strcmp(args[0], "cd") == 0)
		cd(args);
	else if (strcmp(args[0], "ls") == 0)
		ls(args);
	else if (strcmp(args[0], "date") == 0)
		date(args);
	else if (strcmp(args[0], "grep") == 0)
		grep(args);
	else if (strcmp(args[0], "echo") == 0)
		echo(args);
	else if (strcmp(args[0], "history") == 0)
		puts("Print history here");
	else if (strcmp(args[0], "exit") == 0)
		return 1;
	else if (strcmp(args[0], "") == 0)
		return 0;
	else
		puts("command not found");
	
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
ls (char args[][256])
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
			if (ent->d_name[0] != '.')
				printf ("%s\n", ent->d_name);
		}
		closedir (dir);
	}
	free(working_directory);
}

void
date (char args[][256])
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
		printf("Error in date() function, tmptr is NULL");
	} else {
		printf(asctime(currenttm));
	}
}

char*
echo (char args[][256])
{
	// Buffer to return
	char* buffer[256];
	
	int i = 1;
	// If args[i][0] == 0 there's no more text to be echoed
	while (args[i][0] != '\0') {
		// After the first argument we need a space before every argument
		if (i > 1)
			printf(" ");

		printf("%s", args[i]);
		i++;
	}

	// I have to manually do a newline, if there's no text to echo
	if (args[1][0] == '\0')
		printf("\n");
}

void
grep (char args[][256])
{
	// Check whether we got some arguments we shouldn't get or didn't get any arguments at all or got a third arg, but are piping, or did get a third arg, but aren't piping
	// TODO check whether we are piping
	int piping = 0;
	if (args[3][0] != 0 || args[1][0] == 0 || (args[2][0] != 0 && piping) || (args[2][0] == 0 && !piping)) {
		puts("invalid arguments");
		return;
	}
	
	if (piping) {
		// TODO
		search_file(args[1], stdin);
	} else {
		FILE *f;

		// For whatever reason there's a newline
		removeNL(args[2]);

		f = fopen(args[2], "r");
		if (f == NULL) {
			puts("no such file or directory");
			return;
		}
		search_file(args[1], f);
		fclose(f);
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

int
string_to_int (char* string)
{
	return 1;
}


// Search a file for a regex (or rather string)
int
search_file(char *regex, FILE *f)
{
	//int n;
	char buf[BUFSIZ];

	while (fgets(buf, sizeof(buf), f) != NULL) {
		//n = strlen(buf);
		// Avoid printing newlines from the strings we've matched TODO
		/*if (n > 0 && buf[n-1] == '\n')
			buf[n-1] = '\0';*/
		if (search_text(regex, buf))
			printf("%s", buf);
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