#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mpd/client.h>

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

struct termios default_info;

void use_canon (int enable)
{
	if (enable) {
		tcsetattr(0, TCSANOW, &default_info);
	} else {
		struct termios info;
		tcgetattr(0, &info);
		info.c_lflag &= ~ICANON;
		info.c_lflag &= ~ECHO;
		info.c_cc[VMIN] = 1;
		info.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &info);
	}
}

struct mpd_connection *conn = NULL;

int use_mpd (int enable)
{
	if (enable) {
		if (conn) return -1;
		conn = mpd_connection_new(NULL, 0, 0);
		// error handling
		if (conn == NULL) {
			fprintf(stderr, "Out of memory\n");
			exit(3);
		}
		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			const char *m = mpd_connection_get_error_message(conn);
			fprintf(stderr, "%s\n", m);
			mpd_connection_free(conn);
		}
	} else {
		if (conn == NULL) return -2;
		mpd_connection_free(conn);
		conn = NULL;
	}
	return 0;
}

void clean_exit (int code)
{
	use_mpd(0);
	use_canon(1);
	exit(code);
}

void handle_int (int signum)
{
	clean_exit(2);
}

const char *action[] = {
	['a'] = "alfa",
	['b'] = "beta",
	['c'] = "charlie",
	['d'] = "delta",
	['e'] = "echo",
};

const char *translate (char code)
{
	if (code < 0 || code >= LENGTH(action)) return NULL;
	printf("%d: %s\n", code, action[code]);
	return action[code];
}

int main (int argc, char *argv[]) {
	struct sigaction sa = {0};
	sa.sa_handler = &handle_int;
	int res = sigaction(SIGINT, &sa, NULL);
	if (res < 0) exit(1);

	tcgetattr(0, &default_info);
	use_canon(0);
	use_mpd(1);

	puts("Pulsa CTRL-C para salir.");

	// traducir
	int code = 0, size;
	char code_fmt[64];
	while (1) {
		code = getchar();
		translate(code);
	}

	use_mpd(0);
	use_canon(1);
	return 0;
}
