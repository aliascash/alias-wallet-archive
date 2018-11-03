#!groovy

pipeline {
    agent any
    options {
        timestamps()
        timeout(time: 3, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '5'))
        disableConcurrentBuilds()
    }
    environment {
        // In case another branch beside master or develop should be deployed, enter it here
        BRANCH_TO_DEPLOY = 'xyz'
        GITHUB_TOKEN = credentials('cdc81429-53c7-4521-81e9-83a7992bca76')
        SPECTRECOIN_VERSION = '2.2.0'
        DISCORD_WEBHOOK = credentials('991ce248-5da9-4068-9aea-8a6c2c388a19')
    }
    stages {
        stage('Notification') {
            steps {
                // Using result state 'ABORTED' to mark the message on discord with a white border.
                // Makes it easier to distinguish job-start from job-finished
                discordSend(
                        description: "Started build #$env.BUILD_NUMBER",
                        image: '',
                        link: "$env.BUILD_URL",
                        successful: true,
                        result: "ABORTED",
                        thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                        title: "$env.JOB_NAME",
                        webhookURL: "${DISCORD_WEBHOOK}"
                )
            }
        }
        stage('Feature branch') {
            when {
                not {
                    anyOf { branch 'develop'; branch 'master'; branch "${BRANCH_TO_DEPLOY}" }
                }
            }
            parallel {
                stage('Build Debian binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch('Docker/Debian/Dockerfile_noUpload', 'spectreproject/spectre-debian:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
//                stage('Build CentOS binaries') {
//                    agent {
//                        label "docker"
//                    }
//                    steps {
//                        script {
//                            buildFeatureBranch('Docker/CentOS/Dockerfile_noUpload', 'spectreproject/spectre-centos:latest')
//                        }
//                    }
//                    post {
//                        always {
//                            sh "docker system prune --all --force"
//                        }
//                    }
//                }
                stage('Build Fedora binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch('Docker/Fedora/Dockerfile_noUpload', 'spectreproject/spectre-fedora:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                /* Raspi build disabled on all branches different than develop and master to increase build speed
                stage('Build Raspberry Pi binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch('Docker/RaspberryPi/Dockerfile_noUpload', 'spectreproject/spectre-raspi:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                stage('Build Ubuntu binaries') {
                    steps {
                        script {
                            buildFeatureBranch('Docker/Ubuntu/Dockerfile_noUpload', 'spectreproject/spectre-ubuntu:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Mac') {
                    agent {
                        label "mac"
                    }
                    environment {
                        QT_PATH = "${QT_DIR_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.10
                    }
                    stages {
                        stage('Perform MacOS build') {
                            steps {
                                script {
                                    sh "pwd"
                                    sh "./autogen.sh\n" +
                                            "cd db4.8/build_unix/\n" +
                                            "./configure --enable-cxx --disable-shared --disable-replication --with-pic && make\n" +
                                            "cd ../../leveldb/\n" +
                                            "./build_detect_platform build_config.mk ./ && make\n" +
                                            "cd ../\n" +
                                            "qmake src/src.pro -spec macx-clang CONFIG+=x86_64\n" +
                                            "make -j2"
                                }
                            }
                        }
                        stage('Prepare delivery') {
                            steps {
                                script {
                                    def exists = fileExists 'Tor.zip'
                                    if (exists) {
                                        echo 'Archive \'Tor.zip\' exists, nothing to download.'
                                    } else {
                                        echo 'Archive \'Tor.zip\' not found, downloading...'
                                        fileOperations([
                                                fileDownloadOperation(
                                                        password: '',
                                                        targetFileName: 'Tor.zip',
                                                        targetLocation: "${WORKSPACE}",
                                                        url: 'https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Tor.libraries.macOS.zip',
                                                        userName: '')
                                        ])
                                    }
                                    // Unzip Tor and remove debug content
                                    fileOperations([
                                            fileUnZipOperation(
                                                    filePath: "${WORKSPACE}/Tor.zip",
                                                    targetLocation: "${WORKSPACE}/"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/bin/debug"),
                                    ])
                                }
                            }
                        }
                        stage('Create delivery') {
                            steps {
                                script {
                                    sh "./macdeployqt.sh"
// No upload on feature branches, only from develop and master
//                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin.dmg'
//                                }
//                            }
//                        }
//                        stage('Upload delivery') {
//                            agent {
//                                label "housekeeping"
//                            }
//                            steps {
//                                script {
//                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${BRANCH_NAME}/${BUILD_NUMBER}/artifact/Spectrecoin.dmg"
//                                    sh "docker run \\\n" +
//                                            "--rm \\\n" +
//                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
//                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
//                                            "spectreproject/github-uploader:latest \\\n" +
//                                            "github-release upload \\\n" +
//                                            "    --user spectrecoin \\\n" +
//                                            "    --repo spectre \\\n" +
//                                            "    --tag latest \\\n" +
//                                            "    --name \"Spectrecoin-latest-macOS.dmg\" \\\n" +
//                                            "    --file /filesToUpload/Spectrecoin.dmg \\\n" +
//                                            "    --replace"
//                                    sh "rm -f spectrecoin.dmg"
//                                }
//                            }
//                            post {
//                                always {
//                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                    }
                }
                stage('Windows') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                withCredentials([[
                                                         $class           : 'AmazonWebServicesCredentialsBinding',
                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
                                                 ]]) {
                                    sh "docker run --rm \\\n" +
                                            "--env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \\\n" +
                                            "--env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \\\n" +
                                            "--env AWS_DEFAULT_REGION=eu-west-1 \\\n" +
                                            "garland/aws-cli-docker \\\n" +
                                            "aws ec2 start-instances --instance-ids i-06fb7942772e77e55"
                                }
                            }
                        }
                        stage('Prepare build') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                }
                            }
                        }
                        stage('Perform build') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "C:\\Qt\\5.9.6\\msvc2017_64"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                }
                            }
                        }
                        stage('Create delivery') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
                                    createWindowsDelivery('latest')
// No upload on feature branches, only from develop and master
//                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin-latest-WIN64.zip, Spectrecoin-latest-OBFS4-WIN64.zip'
                                }
                            }
                        }
//                        stage('Upload delivery') {
//                            agent {
//                                label "housekeeping"
//                            }
//                            steps {
//                                script {
//                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${BRANCH_NAME}/${BUILD_NUMBER}/artifact/Spectrecoin-latest.zip"
//                                    sh "docker run \\\n" +
//                                            "--rm \\\n" +
//                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
//                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
//                                            "spectreproject/github-uploader:latest \\\n" +
//                                            "github-release upload \\\n" +
//                                            "    --user spectrecoin \\\n" +
//                                            "    --repo spectre \\\n" +
//                                            "    --tag latest \\\n" +
//                                            "    --name \"Spectrecoin-latest-WIN64.zip\" \\\n" +
//                                            "    --file /filesToUpload/Spectrecoin-latest.zip \\\n" +
//                                            "    --replace"
//                                    sh "rm -f Spectrecoin-latest.zip"
//                                }
//                            }
//                            post {
//                                always {
//                                    sh "docker system prune --all --force"
//                                }
//                            }
//                        }
                    }
                }
            }
        }
        stage('Build and upload (develop)') {
            when {
                anyOf { branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            parallel {
                stage('Build Debian binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildDevelopBranch('Docker/Debian/Dockerfile', 'spectreproject/spectre-debian:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
//                stage('Build CentOS binaries') {
//                    agent {
//                        label "docker"
//                    }
//                    steps {
//                        script {
//                            buildDevelopBranch('Docker/CentOS/Dockerfile', 'spectreproject/spectre-centos:latest')
//                        }
//                    }
//                    post {
//                        always {
//                            sh "docker system prune --all --force"
//                        }
//                    }
//                }
                stage('Build Fedora binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildDevelopBranch('Docker/Fedora/Dockerfile', 'spectreproject/spectre-fedora:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Build Raspberry Pi binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildDevelopBranch('Docker/RaspberryPi/Dockerfile', 'spectreproject/spectre-raspi:latest')
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Build Ubuntu binaries') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build Ubuntu binaries') {
                            steps {
                                script {
                                    buildDevelopBranch('Docker/Ubuntu/Dockerfile', 'spectreproject/spectre-ubuntu:latest')
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Docker image build') {
                            steps {
                                build(
                                        job: 'Spectrecoin/docker-spectrecoind/develop',
                                        parameters: [
                                                string(
                                                        name: 'SPECTRECOIN_RELEASE',
                                                        value: 'latest'
                                                )
                                        ]
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
                        QT_PATH = "${QT_DIR_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.10
                    }
                    stages {
                        stage('Perform MacOS build') {
                            steps {
                                script {
                                    sh "pwd"
                                    sh "./autogen.sh\n" +
                                            "cd db4.8/build_unix/\n" +
                                            "./configure --enable-cxx --disable-shared --disable-replication --with-pic && make\n" +
                                            "cd ../../leveldb/\n" +
                                            "./build_detect_platform build_config.mk ./ && make\n" +
                                            "cd ../\n" +
                                            "qmake src/src.pro -spec macx-clang CONFIG+=x86_64\n" +
                                            "make -j2"
                                }
                            }
                        }
                        stage('Prepare delivery') {
                            steps {
                                script {
                                    def exists = fileExists 'Tor.zip'
                                    if (exists) {
                                        echo 'Archive \'Tor.zip\' exists, nothing to download.'
                                    } else {
                                        echo 'Archive \'Tor.zip\' not found, downloading...'
                                        fileOperations([
                                                fileDownloadOperation(
                                                        password: '',
                                                        targetFileName: 'Tor.zip',
                                                        targetLocation: "${WORKSPACE}",
                                                        url: 'https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Tor.libraries.macOS.zip',
                                                        userName: '')
                                        ])
                                    }
                                    // Unzip Tor and remove debug content
                                    fileOperations([
                                            fileUnZipOperation(
                                                    filePath: "${WORKSPACE}/Tor.zip",
                                                    targetLocation: "${WORKSPACE}/"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/bin/debug"),
                                    ])
                                }
                            }
                        }
                        stage('Create delivery') {
                            steps {
                                script {
                                    sh "./macdeployqt.sh"
                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin.dmg'
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "rm -f Spectrecoin.dmg*"
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${BRANCH_NAME}/${BUILD_NUMBER}/artifact/Spectrecoin.dmg"
                                    sh "docker run \\\n" +
                                            "--rm \\\n" +
                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
                                            "spectreproject/github-uploader:latest \\\n" +
                                            "github-release upload \\\n" +
                                            "    --user spectrecoin \\\n" +
                                            "    --repo spectre \\\n" +
                                            "    --tag latest \\\n" +
                                            "    --name \"Spectrecoin-latest-macOS.dmg\" \\\n" +
                                            "    --file /filesToUpload/Spectrecoin.dmg \\\n" +
                                            "    --replace"
                                    sh "rm -f Spectrecoin.dmg*"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                    }
                }
                stage('Windows') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                withCredentials([[
                                                         $class           : 'AmazonWebServicesCredentialsBinding',
                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
                                                 ]]) {
                                    sh "docker run --rm \\\n" +
                                            "--env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \\\n" +
                                            "--env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \\\n" +
                                            "--env AWS_DEFAULT_REGION=eu-west-1 \\\n" +
                                            "garland/aws-cli-docker \\\n" +
                                            "aws ec2 start-instances --instance-ids i-06fb7942772e77e55"
                                }
                            }
                        }
                        stage('Prepare build') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                }
                            }
                        }
                        stage('Perform build') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "C:\\Qt\\5.9.6\\msvc2017_64"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                }
                            }
                        }
                        stage('Create delivery') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
                                    createWindowsDelivery('latest')
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-latest-WIN64.zip, Spectrecoin-latest-OBFS4-WIN64.zip"
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/develop/${BUILD_NUMBER}/artifact/Spectrecoin-latest.zip"
                                    sh "docker run \\\n" +
                                            "--rm \\\n" +
                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
                                            "spectreproject/github-uploader:latest \\\n" +
                                            "github-release upload \\\n" +
                                            "    --user spectrecoin \\\n" +
                                            "    --repo spectre \\\n" +
                                            "    --tag latest \\\n" +
                                            "    --name \"Spectrecoin-latest-WIN64.zip\" \\\n" +
                                            "    --file /filesToUpload/Spectrecoin-latest.zip \\\n" +
                                            "    --replace"
                                    sh "rm -f Spectrecoin-latest.zip"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                    }
                }
            }
        }
        stage('Build and upload (release)') {
            when {
                branch 'master'
            }
            parallel {
                stage('Build Debian binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildMasterBranch('Docker/Debian/Dockerfile', "spectreproject/spectre-debian:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
//                stage('Build CentOS binaries') {
//                    agent {
//                        label "docker"
//                    }
//                    steps {
//                        script {
//                            buildMasterBranch('Docker/CentOS/Dockerfile', "spectreproject/spectre-centos:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
//                        }
//                    }
//                    post {
//                        always {
//                            sh "docker system prune --all --force"
//                        }
//                    }
//                }
                stage('Build Fedora binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildMasterBranch('Docker/Fedora/Dockerfile', "spectreproject/spectre-fedora:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Build Raspberry Pi binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildMasterBranch('Docker/RaspberryPi/Dockerfile', "spectreproject/spectre-raspi:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Build Ubuntu binaries') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build Ubuntu binaries') {
                            steps {
                                script {
                                    buildMasterBranch('Docker/Ubuntu/Dockerfile', "spectreproject/spectre-ubuntu:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Docker image build') {
                            steps {
                                build(
                                        job: 'Spectrecoin/docker-spectrecoind/master',
                                        parameters: [
                                                string(
                                                        name: 'SPECTRECOIN_RELEASE',
                                                        value: "${SPECTRECOIN_VERSION}"
                                                )
                                        ]
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
                        QT_PATH = "${QT_DIR_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.10
                    }
                    stages {
                        stage('Perform MacOS build') {
                            steps {
                                script {
                                    sh "pwd"
                                    sh "./autogen.sh\n" +
                                            "cd db4.8/build_unix/\n" +
                                            "./configure --enable-cxx --disable-shared --disable-replication --with-pic && make\n" +
                                            "cd ../../leveldb/\n" +
                                            "./build_detect_platform build_config.mk ./ && make\n" +
                                            "cd ../\n" +
                                            "qmake src/src.pro -spec macx-clang CONFIG+=x86_64\n" +
                                            "make -j2"
                                }
                            }
                        }
                        stage('Prepare delivery') {
                            steps {
                                script {
                                    def exists = fileExists 'Tor.zip'
                                    if (exists) {
                                        echo 'Archive \'Tor.zip\' exists, nothing to download.'
                                    } else {
                                        echo 'Archive \'Tor.zip\' not found, downloading...'
                                        fileOperations([
                                                fileDownloadOperation(
                                                        password: '',
                                                        targetFileName: 'Tor.zip',
                                                        targetLocation: "${WORKSPACE}",
                                                        url: 'https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Tor.libraries.macOS.zip',
                                                        userName: '')
                                        ])
                                    }
                                    // Unzip Tor and remove debug content
                                    fileOperations([
                                            fileUnZipOperation(
                                                    filePath: "${WORKSPACE}/Tor.zip",
                                                    targetLocation: "${WORKSPACE}/"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/bin/debug"),
                                    ])
                                }
                            }
                        }
                        stage('Create delivery') {
                            steps {
                                script {
                                    sh "./macdeployqt.sh"
                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin.dmg'
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "rm -f Spectrecoin.dmg*"
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${BRANCH_NAME}/${BUILD_NUMBER}/artifact/Spectrecoin.dmg"
                                    sh "docker run \\\n" +
                                            "--rm \\\n" +
                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
                                            "spectreproject/github-uploader:latest \\\n" +
                                            "github-release upload \\\n" +
                                            "    --user spectrecoin \\\n" +
                                            "    --repo spectre \\\n" +
                                            "    --tag ${SPECTRECOIN_VERSION} \\\n" +
                                            "    --name \"Spectrecoin-${SPECTRECOIN_VERSION}-macOS.dmg\" \\\n" +
                                            "    --file /filesToUpload/Spectrecoin.dmg \\\n" +
                                            "    --replace"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                    }
                }
                stage('Windows') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                withCredentials([[
                                                         $class           : 'AmazonWebServicesCredentialsBinding',
                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
                                                 ]]) {
                                    sh "docker run --rm \\\n" +
                                            "--env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \\\n" +
                                            "--env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \\\n" +
                                            "--env AWS_DEFAULT_REGION=eu-west-1 \\\n" +
                                            "garland/aws-cli-docker \\\n" +
                                            "aws ec2 start-instances --instance-ids i-06fb7942772e77e55"
                                }
                            }
                        }
                        stage('Prepare build') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                }
                            }
                        }
                        stage('Perform build') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "C:\\Qt\\5.9.6\\msvc2017_64"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                }
                            }
                        }
                        stage('Create delivery') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
                                    createWindowsDelivery("${SPECTRECOIN_VERSION}")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${SPECTRECOIN_VERSION}-WIN64.zip, Spectrecoin-${SPECTRECOIN_VERSION}-OBFS4-WIN64.zip"
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/master/${BUILD_NUMBER}/artifact/Spectrecoin-${SPECTRECOIN_VERSION}-WIN64.zip"
                                    sh "docker run \\\n" +
                                            "--rm \\\n" +
                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
                                            "spectreproject/github-uploader:latest \\\n" +
                                            "github-release upload \\\n" +
                                            "    --user spectrecoin \\\n" +
                                            "    --repo spectre \\\n" +
                                            "    --tag ${SPECTRECOIN_VERSION} \\\n" +
                                            "    --name \"Spectrecoin-${SPECTRECOIN_VERSION}-WIN64.zip\" \\\n" +
                                            "    --file /filesToUpload/Spectrecoin-${SPECTRECOIN_VERSION}-WIN64.zip \\\n" +
                                            "    --replace"
                                    sh "rm -f Spectrecoin-${SPECTRECOIN_VERSION}-WIN64.zip"
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/master/${BUILD_NUMBER}/artifact/Spectrecoin-${SPECTRECOIN_VERSION}-OBFS4-WIN64.zip"
                                    sh "docker run \\\n" +
                                            "--rm \\\n" +
                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
                                            "spectreproject/github-uploader:latest \\\n" +
                                            "github-release upload \\\n" +
                                            "    --user spectrecoin \\\n" +
                                            "    --repo spectre \\\n" +
                                            "    --tag ${SPECTRECOIN_VERSION} \\\n" +
                                            "    --name \"Spectrecoin-${SPECTRECOIN_VERSION}-OBFS4-WIN64.zip\" \\\n" +
                                            "    --file /filesToUpload/Spectrecoin-${SPECTRECOIN_VERSION}-OBFS4-WIN64.zip \\\n" +
                                            "    --replace"
                                    sh "rm -f Spectrecoin-${SPECTRECOIN_VERSION}-OBFS4-WIN64.zip"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                    }
                }
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
                        link: "$env.BUILD_URL",
                        successful: true,
                        thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                        title: "$env.JOB_NAME",
                        webhookURL: "${DISCORD_WEBHOOK}"
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
                    link: "$env.BUILD_URL",
                    successful: true,
                    result: "UNSTABLE",
                    thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                    title: "$env.JOB_NAME",
                    webhookURL: "${DISCORD_WEBHOOK}"
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
                    link: "$env.BUILD_URL",
                    successful: false,
                    thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                    title: "$env.JOB_NAME",
                    webhookURL: "${DISCORD_WEBHOOK}"
            )
        }
    }
}

def buildFeatureBranch(String dockerfile, String tag) {
    withDockerRegistry(credentialsId: '051efa8c-aebd-40f7-9cfd-0053c413266e') {
        sh "docker build \\\n" +
                "-f $dockerfile \\\n" +
                "--rm \\\n" +
                "-t $tag \\\n" +
                "."
    }
}

def buildDevelopBranch(String dockerfile, String tag) {
    withDockerRegistry(credentialsId: '051efa8c-aebd-40f7-9cfd-0053c413266e') {
        sh "docker build \\\n" +
                "-f $dockerfile \\\n" +
                "--rm \\\n" +
                "--build-arg GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                "--build-arg SPECTRECOIN_REPOSITORY=spectre \\\n" +
                "--build-arg REPLACE_EXISTING_ARCHIVE=--replace \\\n" +
                "-t $tag \\\n" +
                "."
    }
}

def buildMasterBranch(String dockerfile, String tag, String release) {
    withDockerRegistry(credentialsId: '051efa8c-aebd-40f7-9cfd-0053c413266e') {
        sh "docker build \\\n" +
                "-f $dockerfile \\\n" +
                "--rm \\\n" +
                "--build-arg GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                "--build-arg SPECTRECOIN_RELEASE=${release} \\\n" +
                "--build-arg REPLACE_EXISTING_ARCHIVE=--replace \\\n" +
                "-t ${tag} \\\n" +
                "."
    }
}

def prepareWindowsBuild() {
    def exists = fileExists 'Spectre.Prebuild.libraries.zip'

    if (exists) {
        echo 'Archive \'Spectre.Prebuild.libraries.zip\' exists, nothing to download.'
    } else {
        echo 'Archive \'Spectre.Prebuild.libraries.zip\' not found, downloading...'
        fileOperations([
                fileDownloadOperation(
                        password: '',
                        targetFileName: 'Spectre.Prebuild.libraries.zip',
                        targetLocation: "${WORKSPACE}",
                        url: 'https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Prebuild.libraries.win64.zip',
                        userName: ''),
                fileUnZipOperation(
                        filePath: 'Spectre.Prebuild.libraries.zip',
                        targetLocation: '.'),
                folderCopyOperation(
                        destinationFolderPath: 'leveldb',
                        sourceFolderPath: 'Spectre.Prebuild.libraries/leveldb'),
                folderCopyOperation(
                        destinationFolderPath: 'packages64bit',
                        sourceFolderPath: 'Spectre.Prebuild.libraries/packages64bit'),
                folderCopyOperation(
                        destinationFolderPath: 'src',
                        sourceFolderPath: 'Spectre.Prebuild.libraries/src'),
                folderCopyOperation(
                        destinationFolderPath: 'tor',
                        sourceFolderPath: 'Spectre.Prebuild.libraries/tor'),
                folderDeleteOperation(
                        './Spectre.Prebuild.libraries'
                )
        ])
    }
    exists = fileExists 'Tor.zip'
    if (exists) {
        echo 'Archive \'Tor.zip\' exists, nothing to download.'
    } else {
        echo 'Archive \'Tor.zip\' not found, downloading...'
        fileOperations([
                fileDownloadOperation(
                        password: '',
                        targetFileName: 'Tor.zip',
                        targetLocation: "${WORKSPACE}",
                        url: 'https://github.com/spectrecoin/resources/raw/master/resources/Spectrecoin.Tor.libraries.win64.zip',
                        userName: '')
        ])
    }
}

def createWindowsDelivery(String version) {
    // Unzip Tor and remove debug content
    fileOperations([
            fileUnZipOperation(
                    filePath: "${WORKSPACE}/Tor.zip",
                    targetLocation: "${WORKSPACE}/"),
            folderDeleteOperation(
                    folderPath: "${WORKSPACE}/src/bin/debug"),
    ])
    // If directory 'Spectrecoin' exists from brevious build, remove it
    def exists = fileExists "${WORKSPACE}/src/Spectrecoin"
    if (exists) {
        fileOperations([
                folderDeleteOperation(
                        folderPath: "${WORKSPACE}/src/Spectrecoin"),
        ])
    }
    // Rename build directory to 'Spectrecoin' and create directory for content to remove later
    fileOperations([
            folderRenameOperation(
                    source: "${WORKSPACE}/src/bin",
                    destination: "${WORKSPACE}/src/Spectrecoin"),
            folderCreateOperation(
                    folderPath: "${WORKSPACE}/old"),
    ])
    // If archive from previous build exists, move it to directory 'old'
    exists = fileExists "${WORKSPACE}/Spectrecoin.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin.zip"),
        ])
    }
    // If archive from previous build exists, move it to directory 'old'
    exists = fileExists "${WORKSPACE}/Spectrecoin-${version}.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin-${version}.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin-${version}.zip"),
        ])
    }
    // Remove directory with artifacts from previous build
    // Create new delivery archive
    // Rename build directory back to initial name
    fileOperations([
            folderDeleteOperation(
                    folderPath: "${WORKSPACE}/old"),
            fileZipOperation("${WORKSPACE}/src/Spectrecoin")
    ])
    fileOperations([
            fileRenameOperation(
                    source: "${WORKSPACE}/Spectrecoin.zip",
                    destination: "${WORKSPACE}/Spectrecoin-${version}-WIN64.zip"),
            fileRenameOperation(
                    source: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults",
                    destination: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults_plain"),
            fileRenameOperation(
                    source: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults_obfs4",
                    destination: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults"),
            fileZipOperation("${WORKSPACE}/src/Spectrecoin")
    ])
    fileOperations([
            fileRenameOperation(
                    source: "${WORKSPACE}/Spectrecoin.zip",
                    destination: "${WORKSPACE}/Spectrecoin-${version}-OBFS4-WIN64.zip"),
            fileRenameOperation(
                    source: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults",
                    destination: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults_obfs4"),
            fileRenameOperation(
                    source: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults_plain",
                    destination: "${WORKSPACE}/src/Spectrecoin/Tor/torrc-defaults"),
            folderRenameOperation(
                    source: "${WORKSPACE}/src/Spectrecoin",
                    destination: "${WORKSPACE}/src/bin")
    ])
}