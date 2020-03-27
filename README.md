#install deps

#setup yaml
yum install lksctp-tools-devel

#setup rabbitmq
yum install librabbitmq-devel

#setup sqlite3
yum install libsqlite3x-devel

#setup hiredis-vip
git clone https://github.com/vipshop/hiredis-vip.git
cd hiredis-vip/
make
make install


./bootstrap.sh


./configure --prefix=/opt/agc --with-logfiledir=/var/log/agc


make


make install

