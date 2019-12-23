FROM ubuntu:18.04
RUN apt-get update; apt-get install -y curl ffmpeg
RUN curl -sL https://deb.nodesource.com/setup_10.x -o nodesource_setup.sh

RUN bash nodesource_setup.sh
RUN apt-get install -y build-essential nodejs redis-server make gcc-6 g++-6
WORKDIR . /app

COPY package*.json ./

RUN npm install

COPY . .

RUN mv .env.example .env
COPY ./start.sh /
RUN chmod +x /start.sh
CMD ["/start.sh"]

