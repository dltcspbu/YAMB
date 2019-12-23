#!/bin/bash
sleep .$[ ( $RANDOM % 10 ) + 5 ]s
./WebRTCMessageBroker $SIGNALER_HOST $SIGNALER_PORT
