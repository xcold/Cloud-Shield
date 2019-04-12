#! /bin/bash

echo "start Start_Old_Daemon.sh"

cd /usr/Agent-Old
rm -rf /usr/Agent
rm -rf /usr/Agent-Old/Agent.tar
mv /usr/Agent-Old /usr/Agent
cd /usr/Agent
/usr/Agent/Daemon>output_old_daemon 2>&1 & 

echo "end Start_Old_Daemon.sh"
