FROM docker/for-desktop-kernel:5.10.25-6594e668feec68f102a58011bb42bd5dc07a7a9b AS ksrc

FROM ubuntu:20.04
# install kernel sources
WORKDIR /
COPY --from=ksrc /kernel-dev.tar /
RUN tar xf kernel-dev.tar && rm kernel-dev.tar

RUN apt-get -y update && apt-get -y install build-essential time kmod linux-tools-common linux-tools-generic bpftrace  bpfcc-tools
RUN ln -s $(find /usr/lib -name 'perf' | head -n1) /usr/local/bin/perf
#linux-tools-`uname -r`
COPY . /proftest
WORKDIR /proftest
RUN make

CMD mount -t debugfs debugfs /sys/kernel/debug && /bin/bash
