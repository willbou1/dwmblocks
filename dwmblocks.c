#define _GNU_SOURCE
#include <sys/wait.h>
#include<time.h>
#include <fcntl.h>
#include<sys/time.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#ifndef NO_X
#include<X11/Xlib.h>
#endif
#ifdef __OpenBSD__
#define SIGPLUS			SIGUSR1+1
#define SIGMINUS		SIGUSR1-1
#else
#define SIGPLUS			SIGRTMIN
#define SIGMINUS		SIGRTMIN
#endif
#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		80
#define MIN( a, b ) ( ( a < b) ? a : b )
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Block;
#ifndef __OpenBSD__
void dummysighandler(int num);
#endif
void sighandler(int num);
void sigtimeout(int num);
void buttonhandler(int sig, siginfo_t *si, void *ucontext);
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void sighandler(int signum);
int getstatus(char *str, char *last);
void statusloop();
void termhandler();
void pstdout();
#ifndef NO_X
void setroot();
void settimer(double time);
pid_t popen3(char *cmd);
static void (*writestatus) () = setroot;
static int setupX();
static int numscreens;
static Display *dpy;
static Window root;
#else
static void (*writestatus) () = pstdout;
#endif


#include "blocks.h"

static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static char exportstring[CMDLENGTH + 16] = "export BUTTON=-;";
static int button = 0;
static int statusContinue = 1;
static int returnStatus = 0;
static char spaces[11] = "          ";
static int pipefd[2];
static pid_t cpid = 0;


void sigtimeout(int num) {
	kill(-cpid, SIGKILL);
	close(pipefd[1]);
	close(pipefd[0]);
	pipe2(pipefd, O_CLOEXEC);
	write(pipefd[1], "T/O\n", 5);
}

void settimer(double time) {
	struct itimerval timer;
 	timer.it_value.tv_sec = (int)time;
 	timer.it_value.tv_usec = (time - (int)time) * 1000000;
 	timer.it_interval.tv_sec = 0;
 	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
}

pid_t popen3(char *cmd) {
	if (pipe2(pipefd, O_CLOEXEC) == -1)
		return -1;
	int pid;
	if ((pid = fork()) == -1)
		return -1;
	if (pid == 0) {
		close(STDIN_FILENO);
		close(STDERR_FILENO);
		dup2(pipefd[1], STDOUT_FILENO);
		setpgid(0, 0);
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		_exit(127);
	}
	return pid;
}

//opens process *cmd and stores output in *output
void getcmd(const Block *block, char *output)
{
	if (block->signal)
	{
		output[0] = block->signal;
		output++;
	}
	strcpy(output, block->icon);
	char *cmd;

	if (button)
	{
		cmd = strcat(exportstring, block->command);
		cmd[14] = '0' + button;
		cpid = popen3(cmd);
		if (cpid == -1) {
			close(pipefd[0]);
			close(pipefd[1]);
			return;
		}
		cmd[16] = '\0';

	}
	else
	{
		cmd = block->command;
		cpid = popen3(cmd);
		if (cpid == -1) {
			close(pipefd[0]);
			close(pipefd[1]);
			return;
		}
	}
	button = 0;

	settimer(timeout);
	waitpid(cpid, 0, 0);
	settimer(0.0);
	close(pipefd[1]);

	int i = strlen(block->icon);
	read(pipefd[0], output+i, CMDLENGTH-i-delimLen);
	close(pipefd[0]);
	for (char *c = output; *c; c++)
		if (*c == '\n') {
			c[1] = '\0';
			break;
		}
	i = strlen(output);
	if (delim[0] != '\0') {
		//only chop off newline if one is present at the end
		i = output[i-1] == '\n' ? i-1 : i;
		strncpy(output+i, delim, delimLen); 
	}
	else
		output[i++] = '\0';
}

void getcmds(int time)
{
	const Block* current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1)
			getcmd(current,statusbar[i]);
	}
}

void getsigcmds(unsigned int signal)
{
	const Block *current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if (current->signal == signal)
			getcmd(current,statusbar[i]);
	}
	writestatus();
}

void setupsignals()
{
#ifndef __OpenBSD__
	    /* initialize all real time signals with dummy handler */
	for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
		signal(i, dummysighandler);
#endif

	struct sigaction sa;
	sigfillset(&sa.sa_mask);
	sigdelset(&sa.sa_mask, SIGALRM);
	sigdelset(&sa.sa_mask, SIGINT);
	sa.sa_sigaction = sighandler;
	sa.sa_flags = SA_SIGINFO;

	for (unsigned int i = 0; i < LENGTH(blocks); i++)
		if (blocks[i].signal > 0)
			sigaction(SIGMINUS+blocks[i].signal, &sa, NULL);

	sa.sa_sigaction = buttonhandler;
	sigaction(SIGUSR1, &sa, NULL);

	sigaddset(&sa.sa_mask, SIGALRM);
	sa.sa_sigaction = sigtimeout;
	sigaction(SIGALRM, &sa, NULL);

	sa.sa_sigaction = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
}

int getstatus(char *str, char *last)
{
	strcpy(last, str);
	str[0] = '\0';
	strcat(str, spaces);
	for (unsigned int i = 0; i < LENGTH(blocks); i++)
		strcat(str, statusbar[i]);
	str[strlen(str)-strlen(delim)] = '\0';
	strcat(str, spaces);
	return strcmp(str, last);//0 if they are the same
}

#ifndef NO_X
void setroot()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only set root if text has changed.
		return;
	for (unsigned int i = 0; i < numscreens; i++) {
		root = RootWindow(dpy, i);
		XStoreName(dpy, root, statusstr[0]);
		XFlush(dpy);
	}
}

int setupX()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "dwmblocks: Failed to open display\n");
		return 0;
	}
	numscreens = ScreenCount(dpy);
	return 1;
}
#endif

void pstdout()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only write out if text has changed.
		return;
	printf("%s\n",statusstr[0]);
	fflush(stdout);
}


void statusloop()
{
	sigset_t signal_set;
	sigfillset(&signal_set);
	sigdelset(&signal_set, SIGALRM);
	sigdelset(&signal_set, SIGINT);
	setupsignals();
	int i = 0;
	getcmds(-1);
	while (1) {
		sigprocmask(SIG_BLOCK, &signal_set, NULL);
		getcmds(i++);
		writestatus();
		if (!statusContinue)
			break;
		sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
		sleep(1.0);
	}
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum)
{
    return;
}
#endif

void sighandler(int signum)
{
	getsigcmds(signum-SIGPLUS);
	writestatus();
}

void buttonhandler(int sig, siginfo_t *si, void *ucontext)
{
	button = si->si_value.sival_int & 0xff;
	getsigcmds(si->si_value.sival_int >> 8);
	writestatus();
}

void termhandler()
{
	statusContinue = 0;
}

int main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {//Handle command line arguments
		if (!strcmp("-d",argv[i]))
			strncpy(delim, argv[++i], delimLen);
		else if (!strcmp("-p",argv[i]))
			writestatus = pstdout;
	}
#ifndef NO_X
	if (!setupX())
		return 1;
#endif
	delimLen = MIN(delimLen, strlen(delim));
	delim[delimLen++] = '\0';
	spaces[padding] = '\0';
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
#ifndef NO_X
	XCloseDisplay(dpy);
#endif
	return 0;
}
