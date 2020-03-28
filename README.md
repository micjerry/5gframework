#install deps

yum install -y gcc git autoconf automake libtool libyaml-devel lksctp-tools-devel librabbitmq-devel sqlite-devel expat-devel


#setup hiredis-vip
git clone https://github.com/vipshop/hiredis-vip.git
cd hiredis-vip/
make
make install


./bootstrap.sh


./configure --prefix=/opt/agc --with-logfiledir=/var/log/agc


make


make install

