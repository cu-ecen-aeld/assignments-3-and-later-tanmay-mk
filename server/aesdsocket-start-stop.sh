#!/bin/sh

#
# @file name	: aesdsocket-start-stop.sh
#
# @author	: Tanmay Mahendra Kothale
#
# @date	: Feb 20, 2022
#

case "$1" in
    start)	
        echo "Starting aesdsocket"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
        
    stop)
        echo "Stopping aesdsocket" 
        start-stop-daemon -K -n aesdsocket
        ;;
        
    *)
        echo "Usage: $0 {start|stop}" 
        exit 1 
esac

exit 0

#EOF
