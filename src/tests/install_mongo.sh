#!/bin/bash
# VIRTUAL_ENV_NAME=packix_ansible_container

command_exists () {
  type "$1" > /dev/null 2>&1
}

if apt-get --help > /dev/null 2>&1; then
  echo "Found APT-based Linux distro"
	if ! [ $(id -u) = 0 ]; then
		SUDO="sudo"
	fi
  CMD=apt-get
elif dnf --help > /dev/null 2>&1; then
  echo "Found DNF-based Linux distro"
  CMD=dnf
elif yum --help > /dev/null 2>&1; then
  echo "Found YUM-based Linux distro"
  CMD=yum
elif brew --help > /dev/null 2>&1; then
  echo "Found Darwin/OSX"
  CMD=brew
elif [ "$(uname -s)" == "Darwin" ]; then
  echo "Found Darwin/OSX, Not Supported"
	exit 0
else
  echo "Have no idea what OS I'm running on!" >&2
  exit 0
fi


if ! command_exists mongod; then
	if [ "$CMD" == "yum" -o "$CMD" == "dnf" ]; then
		cat <<- EOF > /etc/yum.repos.d/mongodb-org-4.0.repo
			[mongodb-org-4.0]
			name=MongoDB Repository
			baseurl=https://repo.mongodb.org/yum/redhat/$releasever/mongodb-org/4.0/x86_64/
			gpgcheck=1
			enabled=1
			gpgkey=https://www.mongodb.org/static/pgp/server-4.0.asc
		EOF
		$CMD install -y --skip-broken mongodb-org-4.0.9 mongodb-org-server-4.0.9 mongodb-org-shell-4.0.9 mongodb-org-mongos-4.0.9 mongodb-org-tools-4.0.9
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
			exit 0;
		fi

		$SUDO $CMD update
		$SUDO $CMD install install -y mongodb-org=4.0.9 mongodb-org-server=4.0.9 mongodb-org-shell=4.0.9 mongodb-org-mongos=4.0.9 mongodb-org-tools=4.0.9
	else
		echo "Unsupported Platform for Testing"
		exit 0;
	fi
fi