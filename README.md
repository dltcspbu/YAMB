# YAMB

Yet Another Message Broker (YAMB). 
This repository containt distributed message broker on top of WebRTC created to test full MADT (https://github.com/dltcspbu/madt)  functionality.


Building:
1) go to the signaler folder, collect the docker:
docker build -t signaler
2) go to the mongo folder run the script:
. pull.sh
3) go to the dlnode folder, execute the commands:
   docker build -t dltc / libsourcey-base dep_Docker
   docker build -t dltc / dlnode.
Launch:
1) go to the mongo folder:
   . run_mongo.sh
2) execute from an arbitrary folder:
docker run -it --net host signaler
3) go to the dlnode folder, execute
. run_docker.sh

In the dlnode / env file
the address and port of the signler server are recorded, which can be configured before building the signaler docker in the .env.example file, and the address and port of the mongo server can also be configured there.

To add turn \ stun you need:
1) go to the address
http: // SIGNALER_HOST: SIGNALER_PORT / add_ice_server
2) input: turn \ stun: dltc: HOST: PORT
As a HOST, most likely, you need to transfer ip. PORT - port on which stun \ turn server is running

Building docker with its coturn server:
1) Go to the coturn folder and run
docker build -t coturn.
To start your own turn \ stun run the command:
1) docker run --net = host -t coturn
