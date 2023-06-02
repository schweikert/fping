
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "top_view.h"

static char out_buffer[1000];

static int print_top_view_init_printed = 0;
static int next_view_pos = 1;

extern int64_t perhost_interval;


static void pos_printf(uint8_t col, uint8_t row, char* format, ...) __attribute__ ((format (printf, 3, 4)));

static void pos_printf(uint8_t col, uint8_t row, char* format, ...) 
{

    int pos = 0;

    va_list argp;
    va_start(argp, format);


    memset(out_buffer, 0, sizeof(out_buffer));

    pos += sprintf(&out_buffer[pos], "\033[%d;%df", col, row);

    pos += vsnprintf(&out_buffer[pos], sizeof(out_buffer) - pos, format, argp);

    puts( out_buffer );
}

static void line_clear(uint8_t col)
{

    int pos = 0;

    memset(out_buffer, 0, sizeof(out_buffer));

    pos += sprintf(&out_buffer[pos], "\033[%d;%df\033[2K", col, 1);

    puts( out_buffer );
}


void top_view_end( void ){
    // Cursor deaktivieren
    puts( "\033[?25h");
}




#define HOST_POS 2
#define SEND_POS 20
#define RECV_POS 30
#define LOST_POS 40
#define TIMEOUT_POS 50
#define TIMEOUT_TOTAL_POS 60
#define TIMEOUT_SEQ_POS 70
#define TIMEOUT_TIME_POS 80




static void print_top_view_init( void ){
    if ( print_top_view_init_printed == 1 ) return;

    print_top_view_init_printed = 1;

    int pos = 0;
    // Screen löschen und Farbe deaktivieren
    pos += sprintf(&out_buffer[pos], "\033[00m\033[2J");

    // Cursor deaktivieren
    pos += sprintf(&out_buffer[pos], "\033[?25l");

    puts( out_buffer );

    pos_printf( 2 , 10 , "fping top view ( period : %"PRIi64" ms )", ( perhost_interval / 1000 / 1000 ));

    pos_printf( 4 , HOST_POS , "host");
    pos_printf( 4 , SEND_POS , "send");
    pos_printf( 4 , RECV_POS , "recv");
    pos_printf( 4 , LOST_POS , "lost");
    pos_printf( 4 , TIMEOUT_POS , "timeouts: ");
    pos_printf( 4 , TIMEOUT_TOTAL_POS , "total");
    pos_printf( 4 , TIMEOUT_SEQ_POS , "seq");
    pos_printf( 4 , TIMEOUT_TIME_POS , "time");

    
}



void print_top_view( HOST_ENTRY *h, int timeout ) {

    print_top_view_init();

    if ( h->top_view_print_pos == 0 ){
        h->top_view_print_pos = next_view_pos++;
        sprintf(h->last_timeout_time, "0 ms" );

    }

    


    if ( timeout == 1 ){
        h->top_view_last_timeouts++;
        h->top_view_last_timeouts_seq = h->top_view_last_timeouts;

        sprintf(h->last_timeout_time, "%"PRIi64" ms         ", h->top_view_last_timeouts * ( perhost_interval / 1000 / 1000 ) );

    }else{

        if ( h->top_view_last_timeouts > 0 ){
            h->top_view_last_timeouts_count++;
        }

        h->top_view_last_timeouts = 0;
    }

    line_clear( 5 + h->top_view_print_pos );
    
    pos_printf( 5 + h->top_view_print_pos, HOST_POS , "%s", h->host);

    pos_printf( 5 + h->top_view_print_pos, SEND_POS , "%d", h->num_sent);
    pos_printf( 5 + h->top_view_print_pos, RECV_POS , "%d", h->num_recv);
    pos_printf( 5 + h->top_view_print_pos, LOST_POS , "%d", (  h->num_sent - h->num_recv ));
    pos_printf( 5 + h->top_view_print_pos, TIMEOUT_TOTAL_POS , "%"PRIi64, h->top_view_last_timeouts_count);

    pos_printf( 5 + h->top_view_print_pos, TIMEOUT_SEQ_POS , "%"PRIi64, h->top_view_last_timeouts_seq);
    pos_printf( 5 + h->top_view_print_pos, TIMEOUT_TIME_POS , "%s", h->last_timeout_time);
    


}
