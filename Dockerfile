FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake ninja-build pkg-config libcurl4-openssl-dev libsqlite3-dev ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVKAETHER_BUILD_TESTS=OFF && cmake --build build --parallel
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends libcurl4 libsqlite3-0 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/build/vkaether /usr/local/bin/vkaether
COPY config.example.json /app/config.example.json
CMD ["/usr/local/bin/vkaether", "/app/config.json"]
