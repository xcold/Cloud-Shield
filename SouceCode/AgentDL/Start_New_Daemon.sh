echo "start Start_New_Daemon.sh"
chmod 777 /usr/local/Agent
cd /usr/local/Agent/Debug
./Daemon>output 2>&1 &
echo "end Start_New_Daemon.sh"
