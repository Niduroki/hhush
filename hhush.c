#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>

char *get_current_dir_name(void);
int interpret (char* input);
void cd (char* path);
void ls (void);
void date (void);
void echo (char* args);
//int chararray_sizeof (char* arr);
void removeNL(char* string);

int
main()
{
	int run = 1;
	while (run) {
		printf(get_current_dir_name());
		printf(" $ ");
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
		cd(args[1]);
	else if (strcmp(args[0], "ls") == 0)
		ls();
	else if (strcmp(args[0], "date") == 0)
		date();
	else if (strcmp(args[0], "grep") == 0)
		puts("grep something here")
	else if (strcmp(args[0], "echo") == 0)
		echo(args[]);
	else if (strcmp(args[0], "history") == 0)
		puts("Print history here")
	else if (strcmp(args[0], "exit") == 0)
		return 1;
	else if (strcmp(args[0], "\n") == 0)
		return 0;
	else
		return 0;//popen(args[0], "r");//return 0;
		// TODO call the named program here - if there isn't a binary called like the named program print cnf
	
	return 0;
}


void
cd (char* path)
{
	int returncode;
	removeNL(path);
	returncode = chdir(path);
	if (returncode == -1) {
		int errcode = errno;
		//char buffer[300];
		if (errcode == ENOENT) {
			/*strcpy(buffer, path);
			strcat(buffer, " is not a directory\n");
			printf(buffer);*/
			puts("no such file or directory");
		}
	}
}

void
ls (void)
{
	// TODO understand this :|
	// https://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (get_current_dir_name())) != NULL) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_name[0] != '.')
				printf ("%s\n", ent->d_name);
		}
	closedir (dir);
	}
}

void
date (void)
{
	time_t t = time(NULL);
	gmtime(&t);
	struct tm *currenttm = localtime(&t);
	if (currenttm == NULL) {
		printf("Error in date() function, tmptr is NULL");
	} else {
		printf(asctime(currenttm));
	}
}

void
echo (char* args)
{
	int i;
	//printf(args);
	for (i=1; i<127; i++) {
		printf(args[i]);
	}
}

/**
 * Replaces a newline in a string/char array with a string terminating \0
 * Necessary, because splitting inserts a \n for whatever reason
 */
void
removeNL(char* string)
{
	int i = 0;
	while (1) {
		if (string[i] == '\n')
			string[i] = '\0';
		
		if (string[i] == '0')
			return;
		
		i++;
	}
}
