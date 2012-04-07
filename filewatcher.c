#define _GNU_SOURCE
#include <fcntl.h>

#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

int requiredEvent = 0;

void eventHandler( int sig )
{
    requiredEvent = 1;
}

int main( int argc, char *argv[] )
{
    struct stat      statBuf[256];
    struct sigaction action;
    int    fd[256];
    char   dir[256][128];
    char   *slashPtr;
    char   splitFilename[128];
    int    i;

    if( argc < 2 || argc > 256 ) {
        printf( "Usage: %s filename1 [filename2]...\n", argv[0] );
        exit( -1 );
    }

    /* Set up the signal handler */
    action.sa_handler = eventHandler;
    sigemptyset( &action.sa_mask );
    action.sa_flags = SA_RESTART;
    sigaction( SIGRTMIN, &action, NULL );

    for (i=1; i<argc; i++)
    {
        if( lstat( argv[i], &statBuf[i] ) < 0 ) {
            printf( "Cannot lstat %s : %s\n", argv[i], strerror(errno) );
            exit( -1 );
        }

        if( ! S_ISREG( statBuf[i].st_mode ) ) {
            printf( "%s is not a regular file\n", argv[i] );
            exit( -1 );
        }

        /* Find directory the file is in, or assume the current directory */
        strncpy( splitFilename, argv[i], 127 );
        if( (slashPtr = rindex( splitFilename, '/' )) == NULL ) {
            sprintf(dir[i], ".");
        } else {
            *slashPtr = 0;
            sprintf(dir[i], "%s", splitFilename);
        }

        /* Open the directory the file is in */
        if( (fd[i] = open( dir[i], O_RDONLY )) < 0 ) {
            printf( "Cannot open %s for reading : %s\n", dir[i], strerror(errno) );
            exit( -1 );
        }

        /* Choose the signal I want to receive when the directory content changes */
        if( fcntl( fd[i], F_SETSIG, SIGRTMIN) < 0 ) {
            printf( "Cannot set signal : %s\n", strerror(errno) );
            exit( -1 );
        }

        /* Ask for notification when a modification is made in the directory */
        if( fcntl( fd[i], F_NOTIFY, DN_MODIFY|DN_MULTISHOT ) < 0 ) {
            printf( "Cannot fcntl %s : %s\n", dir[i], strerror(errno) );
            exit( -1 );
        }
    }

    /* Infinite loop - a real program would be doing stuff */
    while( 1 ) {
        time_t modifiedTime;

        /* This demo has nothing to do, so just wait for a signal */
        pause();

        /* Check the flag which indicates the right signal has been received */
        if( requiredEvent ) {

            /* Something has been modified in the directory - stat our files */
            for (i=1; i<argc; i++)
            {
                modifiedTime = statBuf[i].st_mtime;

                if( lstat( argv[i], &statBuf[i] ) < 0 ) {
                    break;    /* file disappeared, but continuing anyway... might be just the way an editor saves its files */
                }

                /* If the modification time has changed, the file has been altered */
                if( modifiedTime != statBuf[i].st_mtime ) {
                    printf( "File %s has been modified\n", argv[i] );
                    fflush(stdout);
                }
            }

            requiredEvent = 0;
        }
    }
}
