#!/bin/bash
npm start > signaller.txt 2>&1 &
redis-server --daemonize yes > redis.txt 2>&1 &
sleep 2
tail -f signaller.txt
