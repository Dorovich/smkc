#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mpd/client.h>

/*
  info en:
  https://envs.sh/hEl
  https://envs.sh/hEk
 */

char *translate[] = {
	['a'] = "test"
};

struct mpd_connection *conn = NULL;

struct termios default_info;

void use_canon (int enable) {
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

int use_mpd (int enable) {
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

void handle_int (int signum) {
	puts("SALIENDO...");
	use_mpd(0);
	use_canon(1);
	exit(2);
}

int main (int argc, char *argv[]) {
	struct sigaction sa = {0};
	sa.sa_handler = &handle_int;
	int res = sigaction(SIGINT, &sa, NULL);
	if (res < 0) exit(1);

	tcgetattr(0, &default_info);
	use_canon(0);
	use_mpd(1);

	// repetir carácteres escritos
	int code = 0, size;
	char code_fmt[6];
	while (code == 0) {
		code = getchar();
		size = sprintf(code_fmt, "%s\n", translate[code]);
		write(1, code_fmt, size);
	}

	use_mpd(0);
	use_canon(1);
	return 0;
}
