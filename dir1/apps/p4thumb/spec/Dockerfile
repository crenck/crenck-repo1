FROM ubuntu:16.04

RUN mkdir -p /opt/perforce/server

COPY p4_specs/perforce_user.txt /opt/perforce/server/.
COPY p4_specs/thumb_user.txt /opt/perforce/server/.
COPY p4_specs/test_client.txt /opt/perforce/server/.
COPY p4_specs/thumbnail_client.txt /opt/perforce/server/.

RUN apt-get update -y && apt-get upgrade -y && apt-get install -y wget vim
RUN wget https://swarm.perforce.com/downloads/builds/main/p4-bin/bin.linux26x86_64/p4d && \
	wget https://swarm.perforce.com/downloads/builds/main/p4-bin/bin.linux26x86_64/p4 && \
	chmod +x p4d p4 && \
	mv p4* /usr/local/bin/.
RUN cd /opt/perforce/server && p4d -d && \
    p4 -p 1666 -u perforce user -i < perforce_user.txt && \
    p4 -p 1666 -u thumbuser user -i < thumb_user.txt && \
    p4 -p 1666 -u perforce client -i < thumbnail_client.txt && \
    p4 -p 1666 -u perforce client -i < test_client.txt
EXPOSE 1666
CMD p4d -L /opt/perforce/server/Log -J /opt/perforce/server/journal -r /opt/perforce/server -p 0.0.0.0:1666
