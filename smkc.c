#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mpd/client.h>

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

#define CHECK(f) if (!(f)
#define WARN(msg) fprintf(stderr, "[WARNING] %s\n", msg)
#define ERROR(msg) fprintf(stderr, "[ERROR] %s\n", msg)

#define KEY_CMD(c) { .type = command, .u.cmd = c }
#define KEY_SNG(s) { .type = song, .u.sng = s }

int use_canon (int enable);
int use_mpd (int enable);
void clean_exit (int code);
void handle_int (int signum);
struct key_action *translate (char code);
void run_song (const char *title);

void quit ();
void toggle_pause ();
void hola ();

enum key_action_type { command, song };

struct key_action {
	enum key_action_type type;
	union {
		const char *sng; // título de una pista de audio
		void (*cmd)(); // dirección de una función (comando)
	} u;
};

struct key_action action[] = {
	['p'] = KEY_CMD(toggle_pause),
	['q'] = KEY_CMD(quit),
	['h'] = KEY_CMD(hola),
	['k'] = KEY_SNG("kletka"),
	['s'] = KEY_SNG("kommersanty"),
	['t'] = KEY_SNG("last wish"),
};

int debug = 0;
bool pause_mode = false;
struct termios default_info;
struct mpd_connection *conn = NULL;


/* COMANDOS */

void toggle_pause ()
{
	struct mpd_status *status = mpd_run_status(conn);
	if (!status) {
		ERROR("Could not get status.");
		return;
	}

	enum mpd_state state = mpd_status_get_state(status);
	bool ret;
	switch (state) {
	case MPD_STATE_STOP:
		ret = mpd_run_play(conn);
		break;
	case MPD_STATE_PLAY:
		ret = mpd_run_pause(conn, true);
		break;
	case MPD_STATE_PAUSE:
		ret = mpd_run_pause(conn, false);
		break;
	default:
		break;
	}
	if (!ret) {
		WARN("Could not toggle playback.");
	}
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

void run_song (const char *title) {
	bool ret;
	ret = mpd_run_clear(conn);
	if (!ret) WARN("Could not clear the queue.");

	ret = mpd_search_db_songs(conn, false);
	if (!ret) {
		ERROR("Could not specify db search.");
		return;
	}

	ret = mpd_search_add_tag_constraint(conn,
					MPD_OPERATOR_DEFAULT,
					MPD_TAG_TITLE,
					title);
	if (!ret) {
		ERROR("Could not add search constraint.");
		return;
	}

	ret = mpd_search_commit(conn);
	if (!ret) {
		ERROR("Could not commmit search.");
		return;
	}

	struct mpd_song *s = mpd_recv_song(conn);
	if (!s) {
		ERROR("Search is empty. Song not received.");
		return;
	}

	const char *uri = mpd_song_get_uri(s);
	if (!uri) {
		ERROR("Could not get song uri.");
		return;
	}
	if (debug) fprintf(stdout, "[DEBUG] URI: %s\n", uri);

	ret = mpd_run_add(conn, uri);
	if (!ret) {
		ERROR("Could not add song to queue.");
		return;
	}

	ret = mpd_run_play(conn);
	if (!ret) {
		ERROR("Could not begin playback.");
		return;
	}
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

struct key_action *translate (char code)
{
	if (debug) fprintf(stdout, "[DEBUG] # %d\n", code);
	if (code < 0 || code >= LENGTH(action)) return NULL;

	struct key_action *ka = &action[code];
	if (!ka) return NULL;

	switch (ka->type) {
	case command:
		ka->u.cmd();
		break;
	case song:
		run_song(ka->u.sng);
		break;
	default:
		break;
	}
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
