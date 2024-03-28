# README
TODO:add readme


## CentOS-7 SetUp

### updating git
1. sudo yum groupinstall "Development Tools"
#### Import the GPG key:
2. sudo rpm --import http://opensource.wandisco.com/RPM-GPG-KEY-WANdisco
#If you are using CentOS 7, create a .repo file for the Wandisco repository:
3. sudo sh -c 'echo -e "[WandiscoGit]\nname=Wandisco Git Repository\nbaseurl=http://opensource.wandisco.com/centos/7/git/\$basearch/\nenabled=1\ngpgcheck=1\ngpgkey=http://opensource.wandisco.com/RPM-GPG-KEY-WANdisco" > /etc/yum.repos.d/wandisco-git.repo'
#### Update your repository list:
4.1 sudo yum clean all
4.2 sudo yum update
4.3 sudo yum install git
#### authenticating git
sudo yum install dnf
sudo dnf install 'dnf-command(config-manager)'
sudo dnf config-manager --add-repo https://cli.github.com/packages/rpm/gh-cli.repo
sudo dnf install gh

## Setup CPP
1. sudo yum update
2. sudo yum install centos-release-scl
3. sudo yum install devtoolset-8
（if mirrir connection: sudo yum --disablerepo=elrepo* install centos-release-scl）
4. echo "source scl_source enable devtoolset-8" >> ~/.bashrc

## Install python
sudo yum install libffi-devel
sudo yum install openssl-devel
sudo yum install zlib-devel
sudo yum install bzip2-devel

wget https://www.python.org/ftp/python/3.7.x/Python-3.7.x.tar.xz
tar xvf Python-3.7.x.tar.xz
cd Python-3.7.x
./configure (--enable-optimizations)
make
make altinstall (safe for multiple version of python)

## Jemalloc
1. download from source
2. ./autogen.sh --with-jemalloc-prefix=je _--disable-initial-exec-tls
    make
    make install
 