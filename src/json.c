#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* json_int_to_string(int input) {
    char *result = (char *)malloc(12 * sizeof(char));
    if (result == NULL) {
        perror("Error: Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    sprintf(result, "%d", input);
    return result;
}

void print_json_start(int json_space) {
    for (int i = 0; i < json_space; i++) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "{");
}

void print_json_next(int json_space) {
    fprintf(stderr, ",\n");
    
    for (int i = 0; i < json_space; i++) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "{");
}

void print_json_end(int json_space) {
    fprintf(stderr, "\n");

    for (int i = 0; i < json_space; i++) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "}");
}

void print_json_start_array(const char *json_key, int json_first_record, int json_space) {
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

    if (json_first_record == 0)
        fprintf(stderr, ",\n");
    else
        fprintf(stderr, "\n");

    for (int i = 0; i < json_space; i++) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "\"%s\": [\n", safe_key);

    free(safe_key);
}

void print_json_end_array(int json_space) {
    fprintf(stderr, "\n");

    for (int i = 0; i < json_space; i++) {
        fprintf(stderr, " ");
    }

    fprintf(stderr, "]");
}

void print_json_keyvalue(const char *json_key, const char *json_value, int json_first_record, int json_space) {
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

    if (json_first_record == 0)
        fprintf(stderr, ",\n");
    else
        fprintf(stderr, "\n");

    for (int i = 0; i < json_space; i++){
        fprintf(stderr, " ");
    }

    fprintf(stderr, "\"%s\": \"%s\"", safe_key, safe_value);

    free(safe_key);
    free(safe_value);
}