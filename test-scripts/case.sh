#!/bin/bash

LOG_TO_SYSLOG=1
VERBOSE_OUTPUT=1
LM_VERBOSE="[ 1 = 1 ]"
SERVICE_NAME=test-service

checkCmd ()                                                                                                                                                               
{                                                                                                                                                                         
    cmd=`which $1`;                                                                                                                                                       
    if [ $? -eq 1 ]; then                                                                                                                                                 
        return 1
    fi
    test -x $cmd || return 1;
}

# Function to handle logging
if checkCmd "logger"; then
    LOGGER=`which logger`;
else
    echo "No logger command available" >&2;
fi
# Function to handle logging

log ()                                                                                                                                                                                                                     
{                                                                                                                                                                                                                          
        # $1 should be msg type                                                                                                                                                                                            
        # $2 should be the real msg                                                                                                                                                                                        
        if [ x$LOG_TO_SYSLOG = x1 ]; then                                                                                                                                                                                  
                # NOTE: Add the check on $2 being empty, once you are confident                                                                                                                                            
                # that there aren't any bugs in logging. And no bugs in executing                                                                                                                                          
                # modules and logging                                                                                                                                                                                      
                if [ -x $LOGGER -a "$1" != "STATUS" ]; then                                                                                                                                                                
                        #if [ -z $2 ]; then                                                                                                                                                                                
                        #    continue                                                                                                                                                                                      
                        #elif [ "$1" = "MSG" ]; then                                                                                                                                                                       
                        if [ "$1" = "MSG" ]; then                                                                                                                                                                          
                                logger -p daemon.info -t $SERVICE_NAME "$2";                                                                                                                                                 
                        elif [ "$1" = "ERR" ]; then                                                                                                                                                                        
                                logger -p daemon.err -t $SERVICE_NAME "$2";                                                                                                                                                  
                        elif [ "$1" = "VERBOSE" ]; then                                                                                                                                                                    
                                if [ x$VERBOSE_OUTPUT = x1 ]; then                                                                                                                                                         
                                        logger -p daemon.debug -t $SERVICE_NAME "$2";                                                                                                                                        
                                fi                                                                                                                                                                                         
                        else                                                                                                                                                                                               
                                logger -p daemon.notice -t $SERVICE_NAME "$2";                                                                                                                                               
                        fi                                                                                                                                                                                                 
                fi                                                                                                                                                                                                         
        fi                                                                                                                                                                                                                 
                                                                                                                                                                                                                           
        if [ "$1" = "VERBOSE" ]; then                                                                                                                                                                                      
                $LM_VERBOSE &&  echo "$2" >&2;                                                                                                                                                                             
        elif [ "$1" = "ERR" ]; then                                                                                                                                                                                        
                echo "$2" >&2;                                                                                                                                                                                             
        else                                                                                                                                                                                                               
                # Message of type MSG and STATUS can go to stdout.                                                                                                                                                         
                echo "$2" >&1;                                                                                                                                                                                             
        fi                                                                                                                                                                                                                 
}

case "$1" in
	*)
		log "MSG" "active"
	;;
esac
