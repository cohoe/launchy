FROM fedora:29

RUN dnf update -y
RUN dnf install -y rpmdevtools boost-devel desktop-file-utils \
    gcc-c++ qt-devel make openssh-clients

RUN mkdir -p /root/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

COPY . /opt/launchy

WORKDIR /opt/launchy

#CMD make build
