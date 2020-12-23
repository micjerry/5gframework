#install deps

yum install -y gcc gcc+ gcc-c++ git autoconf automake libtool libyaml-devel lksctp-tools-devel librabbitmq-devel sqlite-devel expat-devel libedit-devel libuuid-devel

#upgrade gcc to 7
yum install -y centos-release-scl
yum install -y devtoolset-7-gcc*
scl enable devtoolset-7 bash

#setup hiredis-vip
git clone https://github.com/vipshop/hiredis-vip.git
cd hiredis-vip/
make
make install

./bootstrap.sh


./configure --prefix=/opt/agc --with-logfiledir=/var/log/agc


make


make install

#edit
touch /etc/ld.so.conf.d/agclibs.conf
echo "/usr/local/lib/" >> /etc/ld.so.conf.d/agclibs.conf
ldconfig
