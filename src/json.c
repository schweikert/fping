#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global variables to control pretty printing */
int json_pretty_print = 2;  /* Default to 2 spaces indent */
static int current_indent = 0;  /* Current indentation level */

static void print_indent(void) {
    if (json_pretty_print > 0) {
        for (int i = 0; i < current_indent * json_pretty_print; i++) {
            fprintf(stderr, " ");
        }
    }
}

static void print_newline(void) {
    if (json_pretty_print > 0) {
        fprintf(stderr, "\n");
    }
}

static void increase_indent(void) {
    if (json_pretty_print > 0) {
        current_indent++;
    }
}

static void decrease_indent(void) {
    if (json_pretty_print > 0 && current_indent > 0) {
        current_indent--;
    }
}

char* json_int_to_string(int input) {
    char *result = (char *)malloc(12 * sizeof(char));
    if (result == NULL) {
        perror("Error: Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    sprintf(result, "%d", input);
    return result;
}

void print_json_start(void) {
    print_indent();
    fprintf(stderr, "{");
    increase_indent();
}

void print_json_next(void) {
    fprintf(stderr, ",");
    print_newline();
    print_indent();
    fprintf(stderr, "{");
    increase_indent();
}

void print_json_end(int json_last_end) {
    decrease_indent();
    print_newline();
    print_indent();
    fprintf(stderr, "}");
    if (json_last_end > 0 && json_pretty_print > 0) {
        print_newline();
    }
}

void print_json_start_array(const char *json_key, int json_first_record) {
    if (json_key == NULL) {
        fprintf(stderr, "Error: NULL pointer provided\n");
        return;
    }

    size_t key_len = strlen(json_key);
    char *safe_key = malloc(key_len + 1);

    if (safe_key == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(safe_key);
        return;
    }

    strcpy(safe_key, json_key);

    if (json_first_record == 0) {
        fprintf(stderr, ",");
        print_newline();
    } else {
        print_newline();
    }

    print_indent();
    if (json_pretty_print > 0) {
        fprintf(stderr, "\"%s\": [", safe_key);
    } else {
        fprintf(stderr, "\"%s\":[", safe_key);
    }

    if (json_first_record > 0)  {
        print_newline();
    }

    increase_indent();

    free(safe_key);
}

void print_json_end_array(void) {
    decrease_indent();
    print_newline();
    print_indent();
    fprintf(stderr, "]");
}

void print_json_keyvalue(const char *json_key, const char *json_value, int json_first_record) {
    if (json_key == NULL || json_value == NULL) {
        fprintf(stderr, "Error: NULL pointer provided\n");
        return;
    }

    size_t key_len = strlen(json_key);
    size_t value_len = strlen(json_value);

    char *safe_key = malloc(key_len + 1);
    char *safe_value = malloc(value_len + 1);

    if (safe_key == NULL || safe_value == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(safe_key);
        free(safe_value);
        return;
    }

    strcpy(safe_key, json_key);
    strcpy(safe_value, json_value);

    if (json_first_record == 0) {
        fprintf(stderr, ",");
        print_newline();
    } else {
        print_newline();
    }

    print_indent();
    fprintf(stderr, "\"%s\": \"%s\"", safe_key, safe_value);

    free(safe_key);
    free(safe_value);
}

void print_json_nokeyvalue(const char *json_value, int json_first_record) {
    if (json_value == NULL) {
        fprintf(stderr, "Error: NULL pointer provided\n");
        return;
    }

    size_t value_len = strlen(json_value);
    char *safe_value = malloc(value_len + 1);

    if (safe_value == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(safe_value);
        return;
    }

    strcpy(safe_value, json_value);

    if (json_first_record == 0) {
        fprintf(stderr, ",");
        print_newline();
    } else if (json_first_record == 1) {
        print_newline();
    }
    print_indent();
    fprintf(stderr, "\"%s\"", safe_value);

    free(safe_value);
}
