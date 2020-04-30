#install deps

yum install -y gcc git autoconf automake libtool libyaml-devel lksctp-tools-devel librabbitmq-devel sqlite-devel expat-devel libedit-devel


#setup hiredis-vip
git clone https://github.com/vipshop/hiredis-vip.git
cd hiredis-vip/
make
make install

# for windows user --slj--2020-0330
# convert ASCII file from dos format to unix format
ls -lrt `find -type f` |awk '{print $9}'|xargs file|grep ASCII| awk '{print $1}'|tr -d ':' |xargs dos2unix

./bootstrap.sh


./configure --prefix=/opt/agc --with-logfiledir=/var/log/agc


make


make install

