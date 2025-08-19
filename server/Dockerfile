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

# RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | bash \
#   && . $NVM_DIR/nvm.sh \
#   && nvm install v22 \
#   && nvm alias default v22 \
#   && nvm use default
RUN curl https://raw.githubusercontent.com/creationix/nvm/v0.40.3/install.sh | bash \
    && . ${HOME}/.nvm/nvm.sh \
    && nvm install v22 \
    && nvm alias default v22 \
    && nvm use default

# ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
# ENV PATH      $NVM_DIR/v$NODE_VERSION/bin:$PATH

WORKDIR /src

COPY . .

WORKDIR /src/frontend

RUN . ${HOME}/.nvm/nvm.sh && npm install && npm run build

FROM ubuntu:24.04 AS runner

RUN apt-get update && apt-get -y upgrade && apt-get install -y wget

RUN wget https://go.dev/dl/go1.25.0.linux-amd64.tar.gz && tar -C /usr/local -xzf go1.25.0.linux-amd64.tar.gz
ENV PATH=$PATH:/usr/local/go/bin

WORKDIR /src

COPY . .

RUN go build

COPY --from=builder /src/build /src/build

CMD [ "./rocket_server" ]
