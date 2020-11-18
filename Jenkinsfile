#!groovy
// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
//
// SPDX-License-Identifier: MIT

pipeline {
    agent {
        label "housekeeping"
    }
    options {
        timestamps()
        timeout(time: 4, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '1'))
        disableConcurrentBuilds()
    }
    environment {
        // In case another branch beside master or develop should be deployed, enter it here
        BRANCH_TO_DEPLOY = "xyz"
        DISCORD_WEBHOOK = credentials('DISCORD_WEBHOOK')
        GITHUB_CI_TOKEN = credentials('GITHUB_CI_TOKEN')
        CI_URL = credentials('CI_URL')
        ACCESS_TOKEN = credentials('ci_read_access')
        DEVELOP_TAG = "Build${BUILD_NUMBER}"
        RELEASE_TAG = sh(
                script: "printf \$(grep CLIENT_VERSION_MAJOR CMakeLists.txt | head -n1 | cut -d ' ' -f2 | sed 's/)//g' | tr -d '\\n' | tr -d '\\r').\$(grep CLIENT_VERSION_MINOR CMakeLists.txt | head -n1 | cut -d ' ' -f2 | sed 's/)//g' | tr -d '\\n' | tr -d '\\r').\$(grep CLIENT_VERSION_REVISION CMakeLists.txt | head -n1 | cut -d ' ' -f2 | sed 's/)//g' | tr -d '\\n' | tr -d '\\r') | sed 's/ //g'",
                returnStdout: true
        )
        GIT_TAG_TO_USE = "${DEVELOP_TAG}"
        GIT_COMMIT_SHORT = sh(
                script: "printf \$(git rev-parse --short ${GIT_COMMIT})",
                returnStdout: true
        )
        CURRENT_DATE = sh(
                script: "printf \"\$(date '+%F %T')\"",
                returnStdout: true
        )
        RELEASE_NAME = "Continuous build #${BUILD_NUMBER} (Branch ${GIT_BRANCH})"
        RELEASE_DESCRIPTION = "Build ${BUILD_NUMBER} from ${CURRENT_DATE}"
        PRERELEASE = "true"
    }
    stages {
        stage('Notification') {
            steps {
                // Using result state 'ABORTED' to mark the message on discord with a white border.
                // Makes it easier to distinguish job-start from job-finished
                discordSend(
                        description: "Started build #$env.BUILD_NUMBER",
                        image: '',
                        //link: "$env.BUILD_URL",
                        successful: true,
                        result: "ABORTED",
                        thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                        title: "$env.JOB_NAME",
                        webhookURL: DISCORD_WEBHOOK
                )
            }
        }
        stage('Feature branch') {
            when {
                not {
                    anyOf { branch 'develop'; branch 'master'; branch "${BRANCH_TO_DEPLOY}" }
                }
            }
            //noinspection GroovyAssignabilityCheck
            parallel {
                stage('Raspberry Pi Buster arm64') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/RaspberryPi/Dockerfile_Buster_noUpload',
                                    dockerTag: "aliascash/alias-wallet-raspi-buster:${GIT_TAG_TO_USE}",
                            )
                            sh "docker rmi aliascash/alias-wallet-raspi-buster:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('CentOS 8') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/CentOS/Dockerfile_noUpload',
                                    dockerTag: "aliascash/alias-wallet-centos-8:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-centos-8:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Debian Stretch') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Debian/Dockerfile_Stretch_noUpload',
                                    dockerTag: "aliascash/alias-wallet-debian-stretch:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-debian-stretch:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Debian Buster') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Debian/Dockerfile_Buster_noUpload',
                                    dockerTag: "aliascash/alias-wallet-debian-buster:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-debian-buster:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Fedora/Dockerfile_noUpload',
                                    dockerTag: "aliascash/alias-wallet-fedora:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-fedora:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('OpenSUSE') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/OpenSUSE/Dockerfile_noUpload',
                                    dockerTag: "aliascash/alias-wallet-opensuse-tumbleweed:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-opensuse-tumbleweed:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Ubuntu 18.04') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Ubuntu/Dockerfile_18_04_noUpload',
                                    dockerTag: "aliascash/alias-wallet-ubuntu-18-04:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-ubuntu-18-04:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Ubuntu 20.04') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Ubuntu/Dockerfile_20_04_noUpload',
                                    dockerTag: "aliascash/alias-wallet-ubuntu-20-04:${GIT_TAG_TO_USE}"
                            )
                            sh "docker rmi aliascash/alias-wallet-ubuntu-20-04:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Mac') {
                    agent {
                        label "mac"
                    }
                    environment {
                        BOOST_PATH = "${BOOST_PATH_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        QT_PATH = "${QT_PATH_MAC_512}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.12
                    }
                    steps {
                        script {
                            sh(
                                    script: """
                                        pwd
                                        ./scripts/cmake-build-mac.sh -g
                                        cp ./cmake-build-cmdline-mac/aliaswallet/Alias.dmg Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg
                                    """
                            )
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                        }
                    }
                }
//                stage('Windows Qt5.12.x') {
//                    stages {
//                        stage('Start Windows slave') {
//                            steps {
//                                withCredentials([[
//                                                         $class           : 'AmazonWebServicesCredentialsBinding',
//                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
//                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
//                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
//                                                 ]]) {
//                                    sh(
//                                            script: """
//                                                docker run \
//                                                    --rm \
//                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
//                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
//                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
//                                                    garland/aws-cli-docker \
//                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
//                                            """
//                                    )
//                                }
//                            }
//                        }
//                        stage('Win + Qt5.12.x') {
//                            agent {
//                                label "windows"
//                            }
//                            environment {
//                                QTDIR = "${QT_DIR_WIN_512}"
//                                VSDIR = "${VS2017_DIR}"
//                                CMAKEDIR = "${CMAKE_DIR}"
//                                VCPKGDIR = "${VCPKG_DIR}"
//                            }
//                            steps {
//                                script {
//                                    bat 'scripts/cmake-build-win.bat'
//                                }
//                            }
//                        }
//                    }
//                }
//                stage('Windows Qt5.15.x') {
//                    stages {
//                        stage('Start Windows slave') {
//                            steps {
//                                withCredentials([[
//                                                         $class           : 'AmazonWebServicesCredentialsBinding',
//                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
//                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
//                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
//                                                 ]]) {
//                                    sh(
//                                            script: """
//                                                docker run \
//                                                    --rm \
//                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
//                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
//                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
//                                                    garland/aws-cli-docker \
//                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
//                                            """
//                                    )
//                                }
//                            }
//                        }
//                        stage('Win + Qt5.15.x') {
//                            agent {
//                                label "windows2"
//                            }
//                            environment {
//                                QTDIR = "${QT_DIR_WIN}"
//                                VSDIR = "${VS2019_DIR}"
//                                CMAKEDIR = "${CMAKE_DIR}"
//                                VCPKGDIR = "${VCPKG_DIR}"
//                            }
//                            steps {
//                                script {
//                                    bat 'scripts/cmake-build-win.bat'
//                                }
//                            }
//                        }
//                    }
//                }
            }
        }
        stage('Prepare master branch build') {
            when {
                branch 'master'
            }
            stages {
                stage('Setup env vars to use') {
                    steps {
                        script {
                            GIT_TAG_TO_USE = "${RELEASE_TAG}"
                            RELEASE_NAME = "Release ${GIT_TAG_TO_USE}"
                            RELEASE_DESCRIPTION = "${WORKSPACE}/ReleaseNotes.md"
                            PRERELEASE = "false"
                        }
                    }
                }
            }
        }
        stage('Git tag handling') {
            when {
                anyOf { branch 'master'; branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            stages {
                stage('Create Git tag') {
                    steps {
                        sshagent(credentials: ['Github-SSH-Login']) {
                            createTag(
                                    tag: "${GIT_TAG_TO_USE}",
                                    commit: "${GIT_COMMIT_SHORT}",
                                    comment: "Created tag ${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                }
                stage('Remove Github release if already existing') {
                    when {
                        expression {
                            return isReleaseExisting(
                                    githubCIToken: GITHUB_CI_TOKEN,
                                    user: 'aliascash',
                                    repository: 'alias-wallet',
                                    tag: "${GIT_TAG_TO_USE}"
                            ) ==~ true
                        }
                    }
                    steps {
                        script {
                            removeRelease(
                                    githubCIToken: GITHUB_CI_TOKEN,
                                    user: 'aliascash',
                                    repository: 'alias-wallet',
                                    tag: "${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                }
                stage('Create Github release') {
                    when {
                        expression {
                            return isReleaseExisting(
                                    githubCIToken: GITHUB_CI_TOKEN,
                                    user: 'aliascash',
                                    repository: 'alias-wallet',
                                    tag: "${GIT_TAG_TO_USE}"
                            ) ==~ false
                        }
                    }
                    steps {
                        script {
                            createRelease(
                                    githubCIToken: GITHUB_CI_TOKEN,
                                    user: 'aliascash',
                                    repository: 'alias-wallet',
                                    tag: "${GIT_TAG_TO_USE}",
                                    name: "${RELEASE_NAME}",
                                    description: "${RELEASE_DESCRIPTION}",
                                    preRelease: "${PRERELEASE}"
                            )
                        }
                    }
                }
            }
        }
        stage('Develop/Master') {
            when {
                anyOf { branch 'master'; branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            //noinspection GroovyAssignabilityCheck
            parallel {
                stage('Raspberry Pi Buster') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Raspberry Pi Buster') {
                            steps {
                                script {
                                    buildBranch(
                                            githubCIToken: GITHUB_CI_TOKEN,
                                            dockerfile: 'Docker/RaspberryPi/Dockerfile_Buster',
                                            dockerTag: "aliascash/alias-wallet-raspi-buster:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "aliascash/alias-wallet-raspi-buster:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Alias-RaspberryPi-Buster-aarch64.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-RaspberryPi-Buster-aarch64.txt"
                                    sh "docker rmi aliascash/alias-wallet-raspi-buster:${GIT_TAG_TO_USE}"
                                }
                            }
                        }
                        stage('Trigger image build') {
                            steps {
                                build(
                                        job: 'Alias/pi-gen/alias_arm64',
                                        parameters: [
                                                string(
                                                        name: 'ALIAS_RELEASE',
                                                        value: "${GIT_TAG_TO_USE}"
                                                ),
                                                string(
                                                        name: 'GIT_COMMIT_SHORT',
                                                        value: "${GIT_COMMIT_SHORT}"
                                                )
                                        ],
                                        wait: false
                                )
                            }
                        }
                    }
                }
                stage('CentOS 8') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('CentOS 8') {
                            steps {
                                script {
                                    buildBranch(
                                            githubCIToken: GITHUB_CI_TOKEN,
                                            dockerfile: 'Docker/CentOS/Dockerfile',
                                            dockerTag: "aliascash/alias-wallet-centos-8:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "aliascash/alias-wallet-centos-8:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Alias-CentOS-8.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-CentOS-8.txt"
                                    sh "docker rmi aliascash/alias-wallet-centos-8:${GIT_TAG_TO_USE}"
                                }
                            }
                        }
                    }
                }
                stage('Debian Stretch') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Debian Stretch') {
                            steps {
                                script {
                                    buildBranch(
                                            githubCIToken: GITHUB_CI_TOKEN,
                                            dockerfile: 'Docker/Debian/Dockerfile_Stretch',
                                            dockerTag: "aliascash/alias-wallet-debian-stretch:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "aliascash/alias-wallet-debian-stretch:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Alias-Debian-Stretch.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-Debian-Stretch.txt"
                                    sh "docker rmi aliascash/alias-wallet-debian-stretch:${GIT_TAG_TO_USE}"
                                }
                            }
                        }
                    }
                }
                stage('Debian Buster') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Debian Buster') {
                            steps {
                                script {
                                    buildBranch(
                                            githubCIToken: GITHUB_CI_TOKEN,
                                            dockerfile: 'Docker/Debian/Dockerfile_Buster',
                                            dockerTag: "aliascash/alias-wallet-debian-buster:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "aliascash/alias-wallet-debian-buster:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Alias-Debian-Buster.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-Debian-Buster.txt"
                                    sh "docker rmi aliascash/alias-wallet-debian-buster:${GIT_TAG_TO_USE}"
                                }
                            }
                        }
                    }
                }
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch(
                                    githubCIToken: GITHUB_CI_TOKEN,
                                    dockerfile: 'Docker/Fedora/Dockerfile',
                                    dockerTag: "aliascash/alias-wallet-fedora:${GIT_TAG_TO_USE}",
                                    gitTag: "${GIT_TAG_TO_USE}",
                                    gitCommit: "${GIT_COMMIT_SHORT}"
                            )
                            getChecksumfileFromImage(
                                    dockerTag: "aliascash/alias-wallet-fedora:${GIT_TAG_TO_USE}",
                                    checksumfile: "Checksum-Alias-Fedora.txt"
                            )
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-Fedora.txt"
                            sh "docker rmi aliascash/alias-wallet-fedora:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('OpenSUSE') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch(
                                    githubCIToken: GITHUB_CI_TOKEN,
                                    dockerfile: 'Docker/OpenSUSE/Dockerfile',
                                    dockerTag: "aliascash/alias-wallet-opensuse-tumbleweed:${GIT_TAG_TO_USE}",
                                    gitTag: "${GIT_TAG_TO_USE}",
                                    gitCommit: "${GIT_COMMIT_SHORT}"
                            )
                            getChecksumfileFromImage(
                                    dockerTag: "aliascash/alias-wallet-opensuse-tumbleweed:${GIT_TAG_TO_USE}",
                                    checksumfile: "Checksum-Alias-OpenSUSE-Tumbleweed.txt"
                            )
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-OpenSUSE-Tumbleweed.txt"
                            sh "docker rmi aliascash/alias-wallet-opensuse-tumbleweed:${GIT_TAG_TO_USE}"
                        }
                    }
                }
                stage('Ubuntu 18.04') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Ubuntu 18.04') {
                            steps {
                                script {
                                    buildBranch(
                                            githubCIToken: GITHUB_CI_TOKEN,
                                            dockerfile: 'Docker/Ubuntu/Dockerfile_18_04',
                                            dockerTag: "aliascash/alias-wallet-ubuntu-18-04:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "aliascash/alias-wallet-ubuntu-18-04:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Alias-Ubuntu-18-04.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-Ubuntu-18-04.txt"
                                    sh "docker rmi aliascash/alias-wallet-ubuntu-18-04:${GIT_TAG_TO_USE}"
                                }
                            }
                        }
                    }
                }
                stage('Ubuntu 20.04') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Ubuntu 20.04') {
                            steps {
                                script {
                                    buildBranch(
                                            githubCIToken: GITHUB_CI_TOKEN,
                                            dockerfile: 'Docker/Ubuntu/Dockerfile_20_04',
                                            dockerTag: "aliascash/alias-wallet-ubuntu-20-04:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "aliascash/alias-wallet-ubuntu-20-04:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Alias-Ubuntu-20-04.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Alias-Ubuntu-20-04.txt"
                                    sh "docker rmi aliascash/alias-wallet-ubuntu-20-04:${GIT_TAG_TO_USE}"
                                }
                            }
                        }
                        stage('Trigger Docker image build') {
                            steps {
                                build(
                                        job: "Alias/docker-aliaswalletd/${GIT_BRANCH}",
                                        parameters: [
                                                string(
                                                        name: 'ALIAS_RELEASE',
                                                        value: "${GIT_TAG_TO_USE}"
                                                ),
                                                string(
                                                        name: 'GIT_COMMIT_SHORT',
                                                        value: "${GIT_COMMIT_SHORT}"
                                                )
                                        ],
                                        wait: false
                                )
                            }
                        }
                    }
                }
                stage('Mac') {
                    agent {
                        label "mac"
                    }
                    environment {
                        BOOST_PATH = "${BOOST_PATH_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        QT_PATH = "${QT_PATH_MAC_512}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.12
                    }
                    stages {
                        stage('MacOS build') {
                            steps {
                                script {
                                    sh(
                                            script: """
                                                pwd
                                                ./scripts/cmake-build-mac.sh -g
                                                cp ./cmake-build-cmdline-mac/aliaswallet/Alias.dmg Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg
                                            """
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                }
                            }
                        }
                        stage('Upload deliveries') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh(
                                            script: """
                                                rm -f Alias*.dmg*
                                                curl -L \
                                                    --user "${ACCESS_TOKEN}" \
                                                    ${CI_URL}/job/Alias/job/alias-wallet/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg \
                                                    --output Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg || true
                                            """
                                    )
                                    uploadArtifactToGitHub(
                                            user: 'aliascash',
                                            repository: 'alias-wallet',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg",
                                    )
                                    createAndArchiveChecksumFile(
                                            filename: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg",
                                            checksumfile: "Checksum-Alias-Mac.txt"
                                    )
                                    sh "rm -f Alias*.dmg* Checksum-Alias*"
                                }
                            }
                        }
                    }
                }
//                stage('Windows Qt5.12.x') {
//                    stages {
//                        stage('Start Windows slave') {
//                            steps {
//                                withCredentials([[
//                                                         $class           : 'AmazonWebServicesCredentialsBinding',
//                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
//                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
//                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
//                                                 ]]) {
//                                    sh(
//                                            script: """
//                                                docker run \
//                                                    --rm \
//                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
//                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
//                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
//                                                    garland/aws-cli-docker \
//                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
//                                            """
//                                    )
//                                }
//                            }
//                        }
//                        stage('Win + Qt5.12.x') {
//                            agent {
//                                label "windows"
//                            }
//                            environment {
//                                QTDIR = "${QT_DIR_WIN_512}"
//                                VSDIR = "${VS2017_DIR}"
//                                CMAKEDIR = "${CMAKE_DIR}"
//                                VCPKGDIR = "${VCPKG_DIR}"
//                            }
//                            steps {
//                                script {
//                                    bat(
//                                        script: """
//                                            if exist build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip del build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip
//                                            scripts/cmake-build-win.bat
//                                        """
//                                        )
//                                    zip(
//                                        zipFile: "${WORKSPACE}/build/Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip",
//                                        dir: "${WORKSPACE}/build",
//                                        glob: "Alias/**"
//                                    )
//                                    bat(
//                                        script: """
//                                            if exist build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip copy build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip
//                                        """
//                                        )
//                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip"
//                                    build(
//                                            job: 'Alias/installer/master',
//                                            parameters: [
//                                                    string(
//                                                            name: 'ARCHIVE_LOCATION',
//                                                            value: "${WORKSPACE}/build"
//                                                    ),
//                                                    string(
//                                                            name: 'ARCHIVE_NAME',
//                                                            value: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.12.zip"
//                                                    ),
//                                                    string(
//                                                            name: 'GIT_TAG_TO_USE',
//                                                            value: "${GIT_TAG_TO_USE}"
//                                                    ),
//                                                    string(
//                                                            name: 'GIT_COMMIT_SHORT',
//                                                            value: "${GIT_COMMIT_SHORT}"
//                                                    )
//                                            ],
//                                            wait: false
//                                    )
//                                }
//                            }
//                        }
//                        stage('Upload deliveries') {
//                            steps {
//                                uploadDeliveries("-Qt5.12")
//                            }
//                        }
//                    }
//                }
//                stage('Windows Qt5.15.x') {
//                    stages {
//                        stage('Start Windows slave') {
//                            steps {
//                                withCredentials([[
//                                                         $class           : 'AmazonWebServicesCredentialsBinding',
//                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
//                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
//                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
//                                                 ]]) {
//                                    sh(
//                                            script: """
//                                                docker run \
//                                                    --rm \
//                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
//                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
//                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
//                                                    garland/aws-cli-docker \
//                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
//                                            """
//                                    )
//                                }
//                            }
//                        }
//                        stage('Win + Qt5.15.x') {
//                            agent {
//                                label "windows2"
//                            }
//                            environment {
//                                QTDIR = "${QT_DIR_WIN}"
//                                VSDIR = "${VS2019_DIR}"
//                                CMAKEDIR = "${CMAKE_DIR}"
//                                VCPKGDIR = "${VCPKG_DIR}"
//                            }
//                            steps {
//                                script {
//                                    bat(
//                                        script: """
//                                            if exist build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip del build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip
//                                            scripts/cmake-build-win.bat
//                                        """
//                                        )
//                                    zip(
//                                        zipFile: "${WORKSPACE}/build/Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip",
//                                        dir: "${WORKSPACE}/build",
//                                        glob: "Alias/**"
//                                    )
//                                    bat(
//                                        script: """
//                                            if exist build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip copy build\\Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip
//                                        """
//                                        )
//                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip"
//                                }
//                            }
//                        }
//                        stage('Upload deliveries') {
//                            steps {
//                                uploadDeliveries("")
//                            }
//                        }
//                    }
//                }
            }
            post {
                always {
                    script {
                        sh(
                            script: """
                                ${WORKSPACE}/scripts/createChecksumSummary.sh \
                                    "${RELEASE_DESCRIPTION}" \
                                    "${WORKSPACE}" \
                                    "${CI_URL}/job/Alias/job/alias-wallet/job/${GIT_BRANCH}/${BUILD_NUMBER}" \
                                    "${ACCESS_TOKEN}"
                            """
                        )
                        editRelease(
                                user: 'aliascash',
                                repository: 'alias-wallet',
                                tag: "${GIT_TAG_TO_USE}",
                                name: "${RELEASE_NAME}",
                                description: "${WORKSPACE}/releaseNotesToDeploy.txt",
                                preRelease: "${PRERELEASE}"
                        )
                        uploadArtifactToGitHub(
                                user: 'aliascash',
                                repository: 'alias-wallet',
                                tag: "${GIT_TAG_TO_USE}",
                                artifactNameLocal: "releaseNotesToDeploy.txt",
                                artifactNameRemote: "RELEASENOTES.txt",
                        )
                    }
                }
            }
        }
        stage('Update download links') {
            when {
                branch 'master'
            }
            steps {
                echo "Update of download links currently disabled"
//                build(
//                        job: 'updateDownloadURLs',
//                        parameters: [
//                                string(
//                                        name: 'RELEASE_VERSION',
//                                        value: "${GIT_TAG_TO_USE}"
//                                ),
//                                string(
//                                        name: 'GIT_COMMIT_SHORT',
//                                        value: "${GIT_COMMIT_SHORT}"
//                                )
//                        ],
//                        wait: false
//                )
            }
        }
    }
    post {
        success {
            script {
                if (!hudson.model.Result.SUCCESS.equals(currentBuild.getPreviousBuild()?.getResult())) {
                    emailext(
                            subject: "GREEN: '${env.JOB_NAME} [${env.BUILD_NUMBER}]'",
                            body: '${JELLY_SCRIPT,template="html"}',
                            recipientProviders: [[$class: 'DevelopersRecipientProvider'], [$class: 'RequesterRecipientProvider']],
//                            to: "to@be.defined",
//                            replyTo: "to@be.defined"
                    )
                }
                discordSend(
                        description: "Build #$env.BUILD_NUMBER finished successfully",
                        image: '',
                        //link: "$env.BUILD_URL",
                        successful: true,
                        thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                        title: "$env.JOB_NAME",
                        webhookURL: DISCORD_WEBHOOK
                )
            }
        }
        unstable {
            emailext(
                    subject: "YELLOW: '${env.JOB_NAME} [${env.BUILD_NUMBER}]'",
                    body: '${JELLY_SCRIPT,template="html"}',
                    recipientProviders: [[$class: 'DevelopersRecipientProvider'], [$class: 'RequesterRecipientProvider']],
//                    to: "to@be.defined",
//                    replyTo: "to@be.defined"
            )
            discordSend(
                    description: "Build #$env.BUILD_NUMBER finished unstable",
                    image: '',
                    //link: "$env.BUILD_URL",
                    successful: true,
                    result: "UNSTABLE",
                    thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                    title: "$env.JOB_NAME",
                    webhookURL: DISCORD_WEBHOOK
            )
        }
        failure {
            emailext(
                    subject: "RED: '${env.JOB_NAME} [${env.BUILD_NUMBER}]'",
                    body: '${JELLY_SCRIPT,template="html"}',
                    recipientProviders: [[$class: 'DevelopersRecipientProvider'], [$class: 'RequesterRecipientProvider']],
//                    to: "to@be.defined",
//                    replyTo: "to@be.defined"
            )
            discordSend(
                    description: "Build #$env.BUILD_NUMBER failed!",
                    image: '',
                    //link: "$env.BUILD_URL",
                    successful: false,
                    thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                    title: "$env.JOB_NAME",
                    webhookURL: DISCORD_WEBHOOK
            )
        }
        aborted {
            discordSend(
                    description: "Build #$env.BUILD_NUMBER was aborted",
                    image: '',
                    //link: "$env.BUILD_URL",
                    successful: true,
                    result: "ABORTED",
                    thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                    title: "$env.JOB_NAME",
                    webhookURL: DISCORD_WEBHOOK
            )
        }
    }
}

def buildWindows(def suffix) {
    prepareWindowsBuild()
    bat 'scripts\\win-genbuild.bat'
    bat 'scripts\\win-build.bat'
    createWindowsDelivery(
            version: "${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}",
            suffix: "${suffix}"
    )
}

def uploadDeliveries(def suffix) {
    script {
        sh(
                script: """
                    rm -f Alias-*-Win64${suffix}.zip
                    wget ${CI_URL}/job/Alias/job/alias-wallet/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64${suffix}.zip
                """
        )
        uploadArtifactToGitHub(
                user: 'aliascash',
                repository: 'alias-wallet',
                tag: "${GIT_TAG_TO_USE}",
                artifactNameRemote: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64${suffix}.zip",
        )
        createAndArchiveChecksumFile(
                filename: "Alias-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64${suffix}.zip",
                checksumfile: "Checksum-Alias-Win64${suffix}.txt"
        )
        sh "rm -f Alias-*-Win64${suffix}.zip Checksum-Alias-Win64${suffix}.txt"
    }
}