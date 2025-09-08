FROM debian:stretch

# Change 1001 to the user ID which you used to checkout this repo, so the permissions will be right.
RUN useradd -u 1001 -m user

RUN echo 'deb http://archive.debian.org/debian stretch main contrib non-free' > /etc/apt/sources.list && \
  echo 'deb http://archive.debian.org/debian-security stretch/updates main contrib non-free' >> /etc/apt/sources.list && \
  apt-get -o Acquire::Check-Valid-Until=false update && \
  apt-get install -y \
    build-essential \
    libncurses5-dev \
    libncursesw5-dev \
    gawk \
    gettext \
    unzip \
    file \
    libssl-dev \
    python \
    python3 \
    rsync \
    time \
    wget \
    git

WORKDIR /home/user/openwrt-kernel49
USER user