/*
 * mod_morse.c -- Morse code ASR TTS Interface
 *
 * Copyright (c) 2019 SignalWire, Inc
 *
 * Author: Anthony Minessale <anthm@signalwire.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <switch.h>
#include <unistd.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_morse_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_morse_shutdown);
SWITCH_MODULE_DEFINITION(mod_morse, mod_morse_load, mod_morse_shutdown, NULL);

struct morse_data {
	teletone_generation_session_t ts;
	switch_buffer_t *audio_buffer;
	float hz;
	int on_dot;
	int on_dash;
	int off;
	int end;
};

typedef struct morse_data morse_t;

static const char* CHAR_TO_MORSE[128] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, "-.-.--", ".-..-.", NULL, NULL, NULL, NULL, ".----.",
	"-.--.", "-.--.-", NULL, NULL, "--..--", "-....-", ".-.-.-", "-..-.",
	"-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
	"---..", "----.", "---...", NULL, NULL, "-...-", NULL, "..--..",
	".--.-.", ".-", "-...", "-.-.", "-..", ".", "..-.", "--.",
	"....", "..", ".---", "-.-", ".-..", "--", "-.", "---",
	".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--",
	"-..-", "-.--", "--..", NULL, NULL, NULL, NULL, "..--.-",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static const char* MORSE_TO_CHAR[128] = {
	NULL, NULL, "E", "T", "I", "N", "A", "M",
	"S", "D", "R", "G", "U", "K", "W", "O",
	"H", "B", "L", "Z", "F", "C", "P", NULL,
	"V", "X", NULL, "Q", NULL, "Y", "J", NULL,
	"5", "6", NULL, "7", NULL, NULL, NULL, "8",
	NULL, "/", NULL, NULL, NULL, "(", NULL, "9",
	"4", "=", NULL, NULL, NULL, NULL, NULL, NULL,
	"3", NULL, NULL, NULL, "2", NULL, "1", "0",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, ":",
	NULL, NULL, NULL, NULL, "?", NULL, NULL, NULL,
	NULL, NULL, "\"", NULL, NULL, NULL, "@", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, "'", NULL,
	NULL, "-", NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, ".", NULL, "_", ")", NULL, NULL,
	NULL, NULL, NULL, ",", NULL, "!", NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const char* char_to_morse(char);
const char* morse_to_char(const char*);
int morse_to_index (const char*);

/*
 * Function morse_to_index by cypherpunks on Reddit.
 * See: http://goo.gl/amr6A3
 */
int morse_to_index (const char* str)
{
	unsigned char sum = 0, bit;

	for (bit = 1; bit; bit <<= 1) {
		switch (*str++) {
		case 0:
			return sum | bit;
		default:
			return 0;
		case '-':
			sum |= bit;
			/* FALLTHROUGH */
		case '.':
			break;
		}
	}

	return 0;
}

const char* char_to_morse (char c)
{
	if (islower(c))
		c += ('A' - 'a');

	return CHAR_TO_MORSE[(int) c];
}

const char* morse_to_char (const char* str)
{
	return MORSE_TO_CHAR[morse_to_index(str)];
}


static const char *text_to_teletone(morse_t *info, switch_stream_handle_t *stream, const char *str)
{

	int i,j;


	for(i = 0; i < strlen(str); i++) {
		const char *code;

		if (str[i] == ' ') {
			stream->write_function(stream, "%%(%d,%d,1)", info->on_dash, info->end);
		} else if ((code = char_to_morse(str[i]))) {
			for (j = 0; j < strlen(code); j++) {
				int on = info->on_dot, off = info->off;

				if (code[j] == '-') {
					on = info->on_dash;
				}

				if (j == strlen(code) -1) {
					off = info->end;
				}

				stream->write_function(stream, "%%(%d,%d,%0.2f)", on, off, info->hz);
			}
		}
	}

	return (const char *)stream->data;
}

static const char *text_to_morse(morse_t *info, switch_stream_handle_t *stream, const char *str)
{

	int i;

	for(i = 0; i < strlen(str); i++) {
		const char *code;

		if (str[i] == ' ') {
			stream->write_function(stream, "%s", " ");
		} else if ((code = char_to_morse(str[i]))) {
			stream->write_function(stream, "%s", code);
		}
	}

	return (const char *)stream->data;
}


static int teletone_handler(teletone_generation_session_t *ts, teletone_tone_map_t *map)
{
	switch_buffer_t *audio_buffer = ts->user_data;
	int wrote;

	if (!audio_buffer) {
		return -1;
	}

	wrote = teletone_mux_tones(ts, map);
	switch_buffer_write(audio_buffer, ts->buffer, wrote * 2);

	return 0;
}

static void init_info(morse_t *info)
{
	info->hz = 1000.0;
	info->on_dot = 60;
	info->on_dash = 120;
	info->off = 100;
	info->end = 500;
}


static switch_status_t morse_speech_open(switch_speech_handle_t *sh, const char *voice_name, int rate, int channels, switch_speech_flag_t *flags)
{
	morse_t *info = switch_core_alloc(sh->memory_pool, sizeof(*info));

	switch_buffer_create_dynamic(&info->audio_buffer, 512, 1024, 0);
	teletone_init_session(&info->ts, 0, teletone_handler, info->audio_buffer);
	info->ts.rate = rate;
	info->ts.channels = channels;

	init_info(info);

	sh->private_info = info;

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t morse_speech_close(switch_speech_handle_t *sh, switch_speech_flag_t *flags)
{
	morse_t *info = (morse_t *) sh->private_info;
	assert(info != NULL);

	switch_buffer_destroy(&info->audio_buffer);
	teletone_destroy_session(&info->ts);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t morse_speech_feed_tts(switch_speech_handle_t *sh, char *text, switch_speech_flag_t *flags)
{
	switch_status_t ret=SWITCH_STATUS_SUCCESS;
	morse_t *info = (morse_t *) sh->private_info;

	switch_stream_handle_t stream = { 0 };

	assert(info != NULL);

	SWITCH_STANDARD_STREAM(stream);


	text_to_teletone(info, &stream, text);
	teletone_run(&info->ts, (char *) stream.data);
	free(stream.data);

	return ret;
}

static switch_status_t morse_speech_read_tts(switch_speech_handle_t *sh, void *data, size_t *datalen, switch_speech_flag_t *flags)
{
	morse_t *info = (morse_t *) sh->private_info;
	size_t read = *datalen;

	assert(info != NULL);

	read = switch_buffer_read(info->audio_buffer, data, read);
	*datalen = read;

	if (read <= 0) {
		return SWITCH_STATUS_BREAK;
	} else {
		return SWITCH_STATUS_SUCCESS;
	}
}

static void morse_speech_flush_tts(switch_speech_handle_t *sh)
{
	morse_t *info = (morse_t *) sh->private_info;
	assert(info != NULL);

	switch_buffer_zero(info->audio_buffer);
}

static void morse_text_param_tts(switch_speech_handle_t *sh, char *param, const char *val)
{
	morse_t *info = (morse_t *) sh->private_info;
	assert(info != NULL);

	if (!(param && val)) return;

	if (!strcasecmp(param, "hz")) {
		info->hz = atof(val);
	} else if (!strcasecmp(param, "on_dot")) {
		info->on_dot = atoi(val);
	} else if (!strcasecmp(param, "on_dash")) {
		info->on_dash = atoi(val);
	} else if (!strcasecmp(param, "off")) {
		info->off = atoi(val);
	} else if (!strcasecmp(param, "end")) {
		info->end = atoi(val);
	}
}

static void morse_numeric_param_tts(switch_speech_handle_t *sh, char *param, int val)
{
	morse_t *info = (morse_t *) sh->private_info;
	assert(info != NULL);

	if (!strcasecmp(param, "hz")) {
		info->hz = val;
	} else if (!strcasecmp(param, "on_dot")) {
		info->on_dot = val;
	} else if (!strcasecmp(param, "on_dash")) {
		info->on_dash = val;
	} else if (!strcasecmp(param, "off")) {
		info->off = val;
	} else if (!strcasecmp(param, "end")) {
		info->end = val;
	}
}

static void morse_float_param_tts(switch_speech_handle_t *sh, char *param, double val)
{
	morse_t *info = (morse_t *) sh->private_info;
	assert(info != NULL);

	if (!strcasecmp(param, "hz")) {
		info->hz = val;
	}
}

#define MORSE_USAGE "<text>"
#define MORSE_API_USAGE "[%]<text>"

SWITCH_STANDARD_APP(morse_function)
{
	morse_t info = { 0 };
	switch_stream_handle_t stream = { 0 };

	SWITCH_STANDARD_STREAM(stream);

	init_info(&info);

	text_to_teletone(&info, &stream, data);
	switch_ivr_gentones(session, (char *)stream.data, 0, NULL);
	free(stream.data);
}

SWITCH_STANDARD_API(morse_api_function)
{
	morse_t info = { 0 };

	if (!cmd) {
		stream->write_function(stream, "%s", "-ERR Missing Text");
		return SWITCH_STATUS_SUCCESS;
	}

	init_info(&info);

	if (*cmd == '%') {
		text_to_teletone(&info, stream, cmd + 1);
	} else {
		text_to_morse(&info, stream, cmd);
	}

	return SWITCH_STATUS_SUCCESS;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_morse_load)
{
	switch_speech_interface_t *speech_interface;
	switch_application_interface_t *app_interface;
	switch_api_interface_t *api_interface;


	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	speech_interface = switch_loadable_module_create_interface(*module_interface, SWITCH_SPEECH_INTERFACE);
	speech_interface->interface_name = "morse";
	speech_interface->speech_open = morse_speech_open;
	speech_interface->speech_close = morse_speech_close;
	speech_interface->speech_feed_tts = morse_speech_feed_tts;
	speech_interface->speech_read_tts = morse_speech_read_tts;
	speech_interface->speech_flush_tts = morse_speech_flush_tts;
	speech_interface->speech_text_param_tts = morse_text_param_tts;
	speech_interface->speech_numeric_param_tts = morse_numeric_param_tts;
	speech_interface->speech_float_param_tts = morse_float_param_tts;


	SWITCH_ADD_APP(app_interface, "morse", "Text to Morse", "Text to Morse", morse_function, MORSE_USAGE, SAF_NONE);
	SWITCH_ADD_API(api_interface, "morse", "Text to Morse / Teletone", morse_api_function, MORSE_API_USAGE);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_morse_shutdown)
{
	return SWITCH_STATUS_UNLOAD;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
