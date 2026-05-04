# Build stage
FROM golang:alpine AS build
RUN apk add --no-cache gcc musl-dev sqlite-dev
WORKDIR /src
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=1 go build -o /out/server ./cmd/server

FROM alpine:3.21
RUN apk add --no-cache ca-certificates sqlite-libs
WORKDIR /app
COPY --from=build /out/server ./server
ENV HTTP_ADDR=:8080 \
    LGTBOT_GRPC_ADDR=unix:///tmp/lgtbot.sock \
    JWT_SECRET=change-me \
    WEB_SQLITE_PATH=/data/web_users.db
EXPOSE 8080
ENTRYPOINT ["./server"]
