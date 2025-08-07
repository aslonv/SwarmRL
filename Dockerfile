FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    python3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /swarmrl

COPY . .

RUN ./build.sh

EXPOSE 8080

CMD ["./build/simple_swarm"]
