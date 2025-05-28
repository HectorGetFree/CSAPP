FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 使用国内apt镜像加速安装
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|https://mirrors.ustc.edu.cn/ubuntu/|g' /etc/apt/sources.list && \
    apt-get clean && apt-get update && \
    apt-get install -y --no-install-recommends build-essential gdb && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]