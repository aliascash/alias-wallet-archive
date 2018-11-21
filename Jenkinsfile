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
        DISCORD_WEBHOOK = credentials('991ce248-5da9-4068-9aea-8a6c2c388a19')
        GITHUB_TOKEN = credentials('cdc81429-53c7-4521-81e9-83a7992bca76')
        GIT_TAG_TO_CREATE = "Build${BUILD_NUMBER}"
        SPECTRECOIN_VERSION = '2.2.0'
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
            //noinspection GroovyAssignabilityCheck
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
                                    prepareMacDelivery()
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
        stage('Develop: Git tag handling') {
            when {
                anyOf { branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            stages {
                stage('Create continuous build tag') {
                    steps {
                        sshagent(credentials: ['df729e83-4f5f-4f8a-b006-031fd8b61c79']) {
                            createTag(
                                    tag: "${GIT_TAG_TO_CREATE}",
                                    commit: "HEAD",
                                    comment: "Created tag ${GIT_TAG_TO_CREATE}"
                            )
                        }
                    }
                }
                stage('Remove CI build if already existing') {
                    when {
                        expression {
                            return isReleaseExisting(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_CREATE}"
                            ) ==~ true
                        }
                    }
                    steps {
                        script {
                            removeRelease(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_CREATE}"
                            )
                        }
                    }
                }
                stage('Create CI build release') {
                    when {
                        expression {
                            return isReleaseExisting(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_CREATE}"
                            ) ==~ false
                        }
                    }
                    steps {
                        script {
                            createRelease(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_CREATE}",
                                    name: "Continuous build No. ${BUILD_NUMBER}",
                                    preRelease: true
                            )
                        }
                    }
                }
            }
        }
        stage('Develop: Build steps') {
            when {
                anyOf { branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            //noinspection GroovyAssignabilityCheck
            parallel {
                stage('Build Debian binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch('Docker/Debian/Dockerfile', 'spectreproject/spectre-debian:latest', "${GIT_TAG_TO_CREATE}")
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
//                            buildBranch('Docker/CentOS/Dockerfile', 'spectreproject/spectre-centos:latest', "${GIT_TAG_TO_CREATE}")
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
                            buildBranch('Docker/Fedora/Dockerfile', 'spectreproject/spectre-fedora:latest', "${GIT_TAG_TO_CREATE}")
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
                            buildBranch('Docker/RaspberryPi/Dockerfile', 'spectreproject/spectre-raspi:latest', "${GIT_TAG_TO_CREATE}")
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
                                    buildBranch('Docker/Ubuntu/Dockerfile', 'spectreproject/spectre-ubuntu:latest', "${GIT_TAG_TO_CREATE}")
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
                                                        value: "${GIT_TAG_TO_CREATE}"
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
                                    prepareMacDelivery()
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
                                            "    --tag ${GIT_TAG_TO_CREATE} \\\n" +
                                            "    --name \"Spectrecoin-${GIT_TAG_TO_CREATE}-macOS.dmg\" \\\n" +
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
                                    createWindowsDelivery("${GIT_TAG_TO_CREATE}")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_CREATE}-WIN64.zip, Spectrecoin-${GIT_TAG_TO_CREATE}-OBFS4-WIN64.zip"
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    uploadArtifactToGitHub("https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/develop/${BUILD_NUMBER}/artifact", "Spectrecoin-${GIT_TAG_TO_CREATE}-WIN64.zip", "${GIT_TAG_TO_CREATE}")
                                    uploadArtifactToGitHub("https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/develop/${BUILD_NUMBER}/artifact", "Spectrecoin-${GIT_TAG_TO_CREATE}-OBFS4-WIN64.zip", "${GIT_TAG_TO_CREATE}")
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
        stage('Master: Git tag handling') {
            when {
                branch 'master'
            }
            stages {
                stage('Create tag') {
                    steps {
                        sshagent (credentials: ['df729e83-4f5f-4f8a-b006-031fd8b61c79']) {
                            createTag(
                                    tag: "${SPECTRECOIN_VERSION}",
                                    commit: "${GIT_COMMIT}",
                                    comment: "Created tag ${SPECTRECOIN_VERSION}"
                            )
                        }
                    }
                }
                stage('Remove Release if already existing') {
                    when {
                        expression {
                            return isReleaseExisting(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${SPECTRECOIN_VERSION}"
                            ) ==~ true
                        }
                    }
                    steps {
                        script {
                            removeRelease(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${SPECTRECOIN_VERSION}"
                            )
                        }
                    }
                }
                stage('Create Release'){
                    when {
                        expression {
                            return isReleaseExisting(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${SPECTRECOIN_VERSION}"
                            ) ==~ false
                        }
                    }
                    steps {
                        script {
                            createRelease(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${SPECTRECOIN_VERSION}",
                                    name: "Release ${SPECTRECOIN_VERSION}",
                                    description: "${WORKSPACE}/ReleaseNotes.md",
                            )
                        }
                    }
                }
            }
        }
        stage('Master: Build steps') {
            when {
                branch 'master'
            }
            //noinspection GroovyAssignabilityCheck
            parallel {
                stage('Build Debian binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch('Docker/Debian/Dockerfile', "spectreproject/spectre-debian:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
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
//                            buildBranch('Docker/CentOS/Dockerfile', "spectreproject/spectre-centos:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
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
                            buildBranch('Docker/Fedora/Dockerfile', "spectreproject/spectre-fedora:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
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
                            buildBranch('Docker/RaspberryPi/Dockerfile', "spectreproject/spectre-raspi:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
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
                                    buildBranch('Docker/Ubuntu/Dockerfile', "spectreproject/spectre-ubuntu:${SPECTRECOIN_VERSION}", "${SPECTRECOIN_VERSION}")
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
                                    prepareMacDelivery()
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
                                    uploadArtifactToGitHub("https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/master/${BUILD_NUMBER}/artifact", "Spectrecoin-${SPECTRECOIN_VERSION}-WIN64.zip", "${SPECTRECOIN_VERSION}")
                                    uploadArtifactToGitHub("https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/master/${BUILD_NUMBER}/artifact", "Spectrecoin-${SPECTRECOIN_VERSION}-OBFS4-WIN64.zip", "${SPECTRECOIN_VERSION}")
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
        aborted {
            discordSend(
                    description: "Build #$env.BUILD_NUMBER was aborted",
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

def buildBranch(String dockerfile, String dockerTag, String gitTag) {
    env.DOCKERFILE = dockerfile
    env.DOCKERTAG = dockerTag
    env.GITTAG = gitTag
    withDockerRegistry(credentialsId: '051efa8c-aebd-40f7-9cfd-0053c413266e') {
        sh(
                script: '''
                    docker build \\
                        -f ${DOCKERFILE} \\
                        --rm \\
                        --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} \\
                        --build-arg SPECTRECOIN_REPOSITORY=spectre \\
                        --build-arg SPECTRECOIN_RELEASE=${GITTAG} \\
                        --build-arg REPLACE_EXISTING_ARCHIVE=--replace \\
                        -t ${DOCKERTAG} \\
                        .
                '''
        )
    }
}

def prepareMacDelivery() {
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
    exists = fileExists "${WORKSPACE}/Spectrecoin-${version}-WIN64.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin-${version}-WIN64.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin-${version}-WIN64.zip"),
        ])
    }
    exists = fileExists "${WORKSPACE}/Spectrecoin-${version}-OBFS4-WIN64.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin-${version}-OBFS4-WIN64.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin-${version}-OBFS4-WIN64.zip"),
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

def uploadArtifactToGitHub(String url, String artifact, String version) {
    sh "wget ${url}/${artifact}"
    sh "docker run \\\n" +
            "--rm \\\n" +
            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
            "-v ${WORKSPACE}:/filesToUpload \\\n" +
            "spectreproject/github-uploader:latest \\\n" +
            "github-release upload \\\n" +
            "    --user spectrecoin \\\n" +
            "    --repo spectre \\\n" +
            "    --tag ${version} \\\n" +
            "    --name \"${artifact}\" \\\n" +
            "    --file /filesToUpload/${artifact} \\\n" +
            "    --replace"
    sh "rm -f ${artifact}"
}
