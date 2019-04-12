#! /bin/bash

echo "start Restart_Agent.sh"

pkill Agent
pkill Daemon

/usr/Agent/Daemon>output_restart 2>&1 &

echo "end Restart_Agent.sh"
