#!/bin/bash
# ============================================================================
#
# SPDX-FileCopyrightText: © 2020 Alias Developers
# SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
# SPDX-License-Identifier: MIT
#
# Docker installation based on
# https://docs.docker.com/install/linux/docker-ce/debian/#install-docker-ce-1
#
# Creation: 2018-07-25 hlxeasy
#
# ============================================================================

# Remove potential old versions
sudo apt-get remove docker docker-engine docker.io containerd runc

# Update the apt package index
sudo apt-get update

# Install packages to allow apt to use a repository over HTTPS
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg-agent \
    software-properties-common

# Add Docker’s official GPG key:
curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add -

# Set up the stable repository
sudo add-apt-repository \
    "deb [arch=amd64] https://download.docker.com/linux/debian \
    $(lsb_release -cs) \
    stable"

# Update the apt package index again
sudo apt-get update

# Install the latest version of Docker CE
sudo apt-get install docker-ce docker-ce-cli containerd.io

# Test installation by running the hello-world-container
sudo docker run --rm hello-world
