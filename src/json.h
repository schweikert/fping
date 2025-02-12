#ifndef _JSON_H
#define _JSON_H

/* Set to 0 for condensed output, positive number for pretty print with that indent level */
extern int json_pretty_print;

char* json_int_to_string(int input);
void print_json_start(void);
void print_json_next(void);
void print_json_end(int json_last_end);
void print_json_start_array(const char *json_key, int json_first_record);
void print_json_end_array(void);
void print_json_keyvalue(const char *json_key, const char *json_value, int json_first_record);
void print_json_nokeyvalue(const char *json_value, int json_first_record);

#endif
