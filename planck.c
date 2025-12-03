/*



                planck
        " Nano, but smaller. And MORE useless.
          And with awful code! "

        The worst text editor to exist. Also,
        the tiniest. One file source code, 272
        lines, and barely any functionality.

        Made in 8 hours.

        Copyright (C) 2025 Havoc Crenshaw.

        This software is licensed under the
        MIT License. See LICENSE for details.



*/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios def_attrs;

static uint64_t idx          = 0;
static uint64_t elems       = 0;
static char*    file_buf    = NULL;
static char*    file_name   = NULL;
static FILE*    file_ptr    = NULL;

static int      exit_code   = EXIT_SUCCESS;
static bool     should_exit = false;

// Print the passed string and mark the application for exit.
static void killErr( const char* str ) {
        puts( str );
        exit_code   = EXIT_FAILURE;
        should_exit = true;
}

// Insert a character at idx into file_buf
static void insertChar( const int chr ) {
        elems++;
        if ( file_buf == NULL ) {
                file_buf = ( char* )malloc( sizeof( char ) );
                if ( file_buf == NULL ) {
                        killErr( "Allocating the file buffer failed." );
                }
        } else {
                file_buf = ( char* )realloc( file_buf,
                                             elems * sizeof( char ) );
                if ( file_buf == NULL ) {
                        killErr(
                                "Reallocating the file buffer failed."
                        );
                }
        }
        if ( idx < elems ) {
                for ( uint64_t i = elems; i > idx; i-- ) {
                        file_buf[i] = file_buf[i-1];
                }
        }
        file_buf[idx] = chr;
        idx++;
}

// Open and read any data from a file, and get it ready for writing.
static void prepFile() {
        bool file_exists = true;

        file_ptr = fopen( file_name, "rb" );
        if ( file_ptr == NULL ) {
                file_exists = false;
        }

        if ( file_exists ) {
                fseek( file_ptr, 0, SEEK_END );
                long file_size = ftell( file_ptr );
                rewind( file_ptr );

                if ( file_size > 0 ) {
                        file_buf = ( char* )malloc( file_size );
                        if ( file_buf == NULL ) {
                                killErr( "Allocating the file buffer failed." );
                        }

                        elems = file_size;
                        idx = elems;
                } else if ( file_size == -1L ) {
                        killErr( "Failed to get size of file." );
                }

                size_t num_bytes = fread( file_buf, sizeof(char),
                                          file_size, file_ptr );
                if ( num_bytes != ( unsigned long )file_size ) { 
                        killErr( "Could not read full file." );
                }
        }

        file_ptr = fopen( file_name, "w" );
        if ( file_ptr == NULL ) {
                killErr( "Failed to open file." );
        }
}

// Store the default terminal mode and enter raw terminal mode.
static void enableRawMode( void ) {
        if ( tcgetattr( STDIN_FILENO, &def_attrs ) != 0 ) {
                killErr( "Could not get default attributes for raw mode" );
        }

        struct termios new_attrs = def_attrs;
        new_attrs.c_iflag       &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON );
        new_attrs.c_oflag       |=  ( OPOST );
        new_attrs.c_cflag       |=  ( CS8 );
        new_attrs.c_lflag       &= ~( ECHO | ICANON | IEXTEN | ISIG );
        new_attrs.c_cc[VMIN]     = 0;
        new_attrs.c_cc[VTIME]    = 1;

        if ( tcsetattr( STDIN_FILENO, TCSAFLUSH, &new_attrs ) != 0 ) {
                killErr( "Could not enable raw mode" );
        }
}

// Exit raw terminal mode.
static void disableRawMode( void ) {
        if ( tcsetattr( STDIN_FILENO, TCSAFLUSH, &def_attrs ) != 0 ) {
                killErr( "Could not disable raw mode" );
        }
}

// Clear the screen and print the current text buffer.
static void refresh() {
        write( STDOUT_FILENO, "\x1b[2J", 4 );
        write( STDOUT_FILENO, "\x1b[H",  3 );
        write( STDOUT_FILENO, file_buf, elems * sizeof(char) );

        write( STDOUT_FILENO, "\x1b[H", 3 );
        for (uint64_t i = 0; i < idx; i++) {
                write( STDOUT_FILENO, "\x1b[C", 3 );
                if ( file_buf[i] == '\n' ) {
                        write( STDOUT_FILENO, "\n", 1 );
                }
                if ( file_buf[i] == '\t' ) {
                        write( STDOUT_FILENO, "\t", 1 );
                }
        }
}

// Read inputs and appropriately push to buffer or run commands.
static void processKey() {
        int num_bytes;
        int chr = '\0';

        while ( ( num_bytes = read( STDIN_FILENO, &chr, 1 ) ) != 1 ) {
                if ( num_bytes == -1 && errno != EAGAIN ) {
                        killErr( "Failed to read input bytes" );
                }
        }

        // C99 doesn't like readability and I don't want a macro or an enum, so,
        // magic numbers. 0x1f is the ctrl key, just handling commands.
        switch ( chr ) {
        
        case 'q' & 0x1f: {
                write( STDOUT_FILENO, "\x1b[2J", 4 );
                write( STDOUT_FILENO, "\x1b[H",  3 );
                should_exit = true;
                break;
        }

        case '\r': {
                insertChar('\n');
                break;
        }

        // Backspace
        case 0x7f: {
                if ( file_buf != NULL ) {
                        for ( uint64_t i = idx; i < elems; i++ ) {
                                file_buf[i] = file_buf[i+1];
                        }
                        if ( idx > 0 ) {
                                idx--;
                        }
                        if ( elems > 0 ) {
                                elems--;
                        }
                }

                break;
        }

        // Escape sequence
        case '\x1b': {
                char esc_code[2];
                if ( read( 0, &esc_code[0], 1 ) != 1 ) {
                        break;
                }
                if ( read( 0, &esc_code[1], 1 ) != 1 ) {
                        break;
                }

                if ( esc_code[0] == '[' ) {
                        switch ( esc_code[1] ) {

                        case 'D': {
                                if ( idx > 0 ) {
                                        idx--;
                                }
                                break;
                        }

                        case 'C': {
                                if ( idx < elems ) {
                                        idx++;
                                }
                                break;
                        }

                        }
                }

                break;
        }

        default: {
                insertChar( chr );
                break;
        }

        }
}

int main( int argc, char** argv ) {
        if ( argc == 2 ) {
                file_name = argv[1];
        } else {
                killErr( "Please specify one file to edit." );
                return exit_code;
        }

        prepFile();

        enableRawMode();
        while ( !should_exit ) {
                refresh();
                processKey();
        }
        disableRawMode();

        if ( file_ptr != NULL ) {
                if ( file_buf != NULL ) {
                        fwrite( file_buf, sizeof(char), elems, file_ptr );
                        free( file_buf );
                }
                fclose( file_ptr );
        }

        return exit_code;
}
