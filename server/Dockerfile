FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y -q --no-install-recommends \
        apt-transport-https \
        build-essential \
        ca-certificates \
        curl \
        git \
        libssl-dev \
        wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

WORKDIR /src/frontend

RUN curl https://raw.githubusercontent.com/creationix/nvm/v0.40.3/install.sh | bash \
    && . ${HOME}/.nvm/nvm.sh \
    && nvm install v22 \
    && nvm alias default v22 \
    && nvm use default \
    && . ${HOME}/.nvm/nvm.sh \
    && npm install \
    && npm run build

WORKDIR /tmp
RUN wget https://go.dev/dl/go1.25.0.linux-amd64.tar.gz && tar -C /usr/local -xzf go1.25.0.linux-amd64.tar.gz
ENV PATH=$PATH:/usr/local/go/bin

WORKDIR /src
RUN go build

FROM ubuntu:24.04

COPY --from=builder /src /src

WORKDIR /src

CMD [ "./rocket_server" ]
