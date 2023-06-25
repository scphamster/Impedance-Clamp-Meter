FROM ubuntu:latest

RUN apt-get update && \
	apt-get install -y \
	gcc-arm-none-eabi \
	libnewlib-arm-none-eabi \
	cmake \
	gdb && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*



