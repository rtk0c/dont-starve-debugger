#!/bin/bash

# socat "PIPE:'$1/data/debugger_bound'!!PIPE:'$1/data/debuggee_bound'" "TCP-CONNECT:127.0.0.1:28544"

cd "$1/data"
socat TCP-CONNECT:127.0.0.1:28544 UNIX-LISTEN:debugger

# mkfifo "$1/data/debugger_bound"
# mkfifo "$1/data/debuggee_bound"
# socat TCP-CONNECT:127.0.0.1:28544 "PIPE:$1/data/debugg    er_bound" &
# socat "PIPE:$1/data/debuggee_bound" TCP-CONNECT:127.0.0.1:28544 &
# rm "$1/data/debugger_bound"
# rm "$1/data/debuggee_bound"