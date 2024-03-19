#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mpd/client.h>

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

int use_canon (int enable);
int use_mpd (int enable);
void clean_exit (int code);
void handle_int (int signum);
union key_action *translate (char code);

void quit ();
void toggle_pause ();
void hola ();

union key_action {
	const char *snd; // ruta de un archivo de audio
	void (*cmd)(); // dirección de una función
};

union key_action action[] = {
	['p'] = { .cmd = toggle_pause },
	['q'] = { .cmd = quit },
	['h'] = { .cmd = hola },
	/* ['t'] = { .snd = "~/Music/test.mp3" }, */
};

int debug = 0;
bool pause_mode = true;
struct termios default_info;
struct mpd_connection *conn = NULL;


/* COMANDOS */

void toggle_pause ()
{
	if (!mpd_run_pause(conn, pause_mode))
		clean_exit(10);
	else pause_mode = !pause_mode;
}

void quit ()
{
	clean_exit(0);
}

void hola ()
{
	fprintf(stdout, "hola wenas\n");
}


/* FUNCIONES INTERNAS */

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

int use_canon (int enable)
{
	if (enable) {
		return tcsetattr(0, TCSANOW, &default_info);
	} else {
		struct termios info;
		tcgetattr(0, &info);
		info.c_lflag &= ~ICANON;
		info.c_lflag &= ~ECHO;
		info.c_cc[VMIN] = 1;
		info.c_cc[VTIME] = 0;
		return tcsetattr(0, TCSANOW, &info);
	}
}

int use_mpd (int enable)
{
	if (enable) {
		if (conn) return -2;
		conn = mpd_connection_new(NULL, 0, 0);
		// error handling
		if (conn == NULL) {
			fprintf(stderr, "[ERROR] Out of memory\n");
			return -1;
		}
		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			const char *m = mpd_connection_get_error_message(conn);
			fprintf(stderr, "[ERROR] %s\n", m);
			mpd_connection_free(conn);
			conn = NULL;
			return -1;
		}
	} else {
		if (conn == NULL) return -2;
		mpd_connection_free(conn);
		conn = NULL;
	}
	return 0;
}

union key_action *translate (char code)
{
	if (debug) fprintf(stdout, "[DEBUG] # %d\n", code);
	if (code < 0 || code >= LENGTH(action)) return NULL;
	union key_action *ka = &action[code];
	if (ka && ka->cmd) ka->cmd();
	return ka;
}

int main (int argc, char *argv[])
{
	struct sigaction sa = {0};
	sa.sa_handler = &handle_int;
	int res = sigaction(SIGINT, &sa, NULL);
	if (res < 0) exit(1);

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-d")) debug = 1;
	}

	tcgetattr(0, &default_info);
	if (use_canon(0) < 0 || use_mpd(1) < 0)
		clean_exit(1);

	fprintf(stdout, "Pulsa CTRL-C para salir.\n");

	// traducir
	while (1) translate(getchar());

	clean_exit(0);
}
