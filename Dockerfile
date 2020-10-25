FROM ubuntu:latest
RUN apt-get -y update
RUN apt-get -y install gcc
RUN apt-get -y install libc-dev
RUN apt-get -y install make
RUN apt-get -y install libreadline-dev
