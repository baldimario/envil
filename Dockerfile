FROM alpine:3.21 AS base
WORKDIR /app

FROM base AS install
RUN apk update && apk upgrade
RUN apk add gcc libc-dev make json-c-dev yaml-dev

FROM install AS build
COPY ./ /app
RUN make clean && make
RUN cp /app/bin/envil /tmp
RUN rm -rf /app/*
RUN mv /tmp/envil /app

FROM build AS latest
CMD ["/app/envil"]