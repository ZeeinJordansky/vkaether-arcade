FROM mcr.microsoft.com/dotnet/sdk:8.0 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake ninja-build pkg-config libcurl4-openssl-dev libsqlite3-dev liblua5.4-dev clang zlib1g-dev cargo rustc ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVKAETHER_BUILD_TESTS=OFF && cmake --build build --parallel 2

FROM golang:1.23-bookworm AS go-build
WORKDIR /src
COPY go.mod ./
COPY cmd ./cmd
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /out/aetherd ./cmd/aetherd

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends libcurl4 libsqlite3-0 liblua5.4-0 libstdc++6 libgcc-s1 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/build/vkaether /usr/local/bin/vkaether
COPY --from=build /app/build/csharp/libAetherPolicy.so /usr/local/lib/libAetherPolicy.so
COPY --from=go-build /out/aetherd /usr/local/bin/aetherd
COPY config.example.json /app/config.example.json
RUN ldconfig
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/aetherd"]
CMD ["-binary", "/usr/local/bin/vkaether", "-config", "/app/config.json", "-listen", ":8080"]
