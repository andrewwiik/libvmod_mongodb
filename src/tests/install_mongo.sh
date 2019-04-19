#!/bin/bash

# "$@"
# status=$?
# # echo "FINAL STATUS: $status"
# exit $status

command_exists () {
  type "$1" > /dev/null 2>&1
}

SKIP=0

if apt-get --help > /dev/null 2>&1; then
  #echo "Found APT-based Linux distro"
	if ! [ $(id -u) = 0 ]; then
		SUDO="sudo"
	fi
  CMD=apt-get
elif dnf --help > /dev/null 2>&1; then
  #echo "Found DNF-based Linux distro"
  CMD=dnf
	SUDO="sudo"
elif yum --help > /dev/null 2>&1; then
  #echo "Found YUM-based Linux distro"
  CMD=yum
	SUDO="sudo"
elif brew --help > /dev/null 2>&1; then
 	#echo "Found Darwin/OSX, Not Supported"
	SKIP=0
elif [ "$(uname -s)" == "Darwin" ]; then
  #echo "Found Darwin/OSX, Not Supported"
	SKIP=0
else
  #echo "Have no idea what OS I'm running on!" >&2
  SKIP=0
fi


if ! command_exists mongod; then
	if [ "$CMD" == "yum" -o "$CMD" == "dnf" ]; then
		$SUDO cat <<- EOF > /etc/yum.repos.d/mongodb-org-4.0.repo
			[mongodb-org-4.0]
			name=MongoDB Repository
			baseurl=https://repo.mongodb.org/yum/redhat/\$releasever/mongodb-org/4.0/x86_64/
			gpgcheck=1
			enabled=1
			gpgkey=https://www.mongodb.org/static/pgp/server-4.0.asc
		EOF

		$SUDO $CMD install -y --skip-broken mongodb-org-4.0.9 mongodb-org-server-4.0.9 mongodb-org-shell-4.0.9 mongodb-org-mongos-4.0.9 mongodb-org-tools-4.0.9
	elif [ "$CMD" == "apt-get" ]; then
		$SUDO $CMD update
		$SUDO apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 9DA31620334BD75D9DCB49F368818C72E52529D4
		UBUNTU_VERSION="0"
		if [ -f /etc/lsb-release ]; then
			. /etc/lsb-release
			UBUNTU_VERSION="$DISTRIB_RELEASE"
		else
			if [ -f /etc/os-release ]; then
				. /etc/os-release
				UBUNTU_VERSION="$VERSION_ID"
			fi
		fi

		UBUNTU_VERSION="${UBUNTU_VERSION//./}"
		if [ "$UBUNTU_VERSION" -ge "1804" ]; then
			echo "deb [ arch=amd64 ] https://repo.mongodb.org/apt/ubuntu bionic/mongodb-org/4.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.0.list
		elif [ "$UBUNTU_VERSION" -ge "1604" ]; then
			echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu xenial/mongodb-org/4.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.0.list
		elif [ "$UBUNTU_VERSION" -ge "1404" ]; then
			echo "deb [ arch=amd64 ] https://repo.mongodb.org/apt/ubuntu trusty/mongodb-org/4.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.0.list
		else
			echo "Unsupported Ubuntu Version"
			SKIP=1;
		fi

		$SUDO $CMD update
		$SUDO $CMD install install -y mongodb-org=4.0.9 mongodb-org-server=4.0.9 mongodb-org-shell=4.0.9 mongodb-org-mongos=4.0.9 mongodb-org-tools=4.0.9
	else
		#echo "Unsupported Platform for Testing"
		SKIP=0
	fi
fi

if command_exists mongod; then
	CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
	mkdir -p "$CURRENT_DIR/tmp"
	mkdir -p "$CURRENT_DIR/tmp/data"
	mkdir -p "$CURRENT_DIR/tmp/logs"
	touch "$CURRENT_DIR/tmp/logs/mongodb.log"

	mongod --fork --logpath "$CURRENT_DIR/tmp/logs/mongodb.log" --dbpath "$CURRENT_DIR/tmp/data"  > /dev/null 2>&1
	# Wait until mongo is ready (or timeout after 60s)
	COUNTER=0
	until mongo --eval "print(\"waited for connection\")" > /dev/null 2>&1
  do
    sleep 2
		(( COUNTER+=2 ))
		#echo "Waiting for mongo to initialize... ($COUNTER seconds so far)"
		if [ $COUNTER -gt 60 ]; then
			#echo "Mongo didn't start within 60 seconds"
			break
		fi
  done
fi

# set -ex
# "$@"
# status=$?
# # echo "FINAL STATUS: $status"
# exit $status