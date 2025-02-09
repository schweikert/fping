#ifndef _JSON_H
#define _JSON_H

char* json_int_to_string(int input);
void print_json_start(int json_space);
void print_json_next(int json_space);
void print_json_end(int json_space);
void print_json_start_array(const char *json_key, int json_first_record, int json_space);
void print_json_end_array(int json_space);
void print_json_keyvalue(const char *json_key, const char *json_value, int json_first_record, int json_space);

#endif