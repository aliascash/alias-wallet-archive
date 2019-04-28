#!groovy

pipeline {
    agent {
        label "docker"
    }
    options {
        timestamps()
        timeout(time: 3, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '1'))
        disableConcurrentBuilds()
    }
    environment {
        // In case another branch beside master or develop should be deployed, enter it here
        BRANCH_TO_DEPLOY = 'xyz'
        DISCORD_WEBHOOK = credentials('991ce248-5da9-4068-9aea-8a6c2c388a19')
        GITHUB_TOKEN = credentials('cdc81429-53c7-4521-81e9-83a7992bca76')
        DEVELOP_TAG = "Build${BUILD_NUMBER}"
        RELEASE_TAG = '3.0.8'
        BLOCKCHAIN_ARCHIVE_VERSION = "2019-04-26"
        GIT_TAG_TO_USE = "${DEVELOP_TAG}"
        GIT_COMMIT_SHORT = sh(
                script: "printf \$(git rev-parse --short ${GIT_COMMIT})",
                returnStdout: true
        )
        CURRENT_DATE = sh(
                script: "printf \"\$(date '+%F %T')\"",
                returnStdout: true
        )
        RELEASE_NAME = "Continuous build No. ${BUILD_NUMBER}"
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
                    steps {
                        script {
                            buildFeatureBranch('Docker/Debian/Dockerfile_noUpload', "spectreproject/spectre-debian:${GIT_TAG_TO_USE}")
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
                            buildFeatureBranch('Docker/RaspberryPi/Dockerfile_noUpload', "spectreproject/spectre-raspi:${GIT_TAG_TO_USE}")
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                /* CentOS build disabled, not working at the moment
                stage('Build CentOS binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch('Docker/CentOS/Dockerfile_noUpload', "spectreproject/spectre-centos:${GIT_TAG_TO_USE}")
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                stage('Build Fedora binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch('Docker/Fedora/Dockerfile_noUpload', "spectreproject/spectre-fedora:${GIT_TAG_TO_USE}")
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
                    steps {
                        script {
                            buildFeatureBranch('Docker/Ubuntu/Dockerfile_noUpload', "spectreproject/spectre-ubuntu:${GIT_TAG_TO_USE}")
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
                                    sh "./scripts/mac-build.sh"
                                }
                            }
                        }
                        stage('Prepare plain delivery') {
                            steps {
                                script {
                                    sh 'rm -f Spectrecoin*.dmg'
                                    prepareMacDelivery()
                                }
                            }
                        }
                        stage('Create plain delivery') {
                            steps {
                                script {
                                    sh "./scripts/mac-deployqt.sh"
                                    sh "mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                    // The following archive step is only for development purposes.
                                    // Remove it before merge to develop!
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                }
                            }
                        }
                        stage('Prepare OBFS4 delivery') {
                            steps {
                                script {
                                    prepareMacOBFS4Delivery()
                                }
                            }
                        }
                        stage('Create OBFS4 delivery') {
                            steps {
                                script {
                                    sh "./scripts/mac-deployqt.sh"
                                    sh "mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg"
                                }
                            }
                        }
                    }
                }
                stage('Windows-Qt5.9.6') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
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
                                QTDIR = "${QT_DIR_WIN}"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-genbuild.bat'
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
                                    createWindowsDelivery("${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}", "-Qt5.9.6")
                                    // The following archive step is only for development purposes.
                                    // Remove it before merge to develop!
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6.zip, Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6-OBFS4.zip"
                                }
                            }
                        }
                    }
                }
                stage('Windows-Qt5.12.0') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
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
                                label "windows2"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                }
                            }
                        }
                        stage('Perform build') {
                            agent {
                                label "windows2"
                            }
                            environment {
                                QTDIR = "${QT512_DIR_WIN}"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-genbuild.bat'
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                }
                            }
                        }
                        stage('Create delivery') {
                            agent {
                                label "windows2"
                            }
                            steps {
                                script {
                                    createWindowsDelivery("${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}", "")
                                    // The following archive step is only for development purposes.
                                    // Remove it before merge to develop!
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip, Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip"
                                }
                            }
                        }
                    }
                }
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
                        sshagent(credentials: ['f06ad0d1-a5e8-41f1-a48e-e877303770b9']) {
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
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_USE}"
                            ) ==~ true
                        }
                    }
                    steps {
                        script {
                            removeRelease(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                }
                stage('Create Github release') {
                    when {
                        expression {
                            return isReleaseExisting(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
                                    tag: "${GIT_TAG_TO_USE}"
                            ) ==~ false
                        }
                    }
                    steps {
                        script {
                            createRelease(
                                    user: 'spectrecoin',
                                    repository: 'spectre',
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
        stage('Build steps') {
            when {
                anyOf { branch 'master'; branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            //noinspection GroovyAssignabilityCheck
            parallel {
                stage('Build Raspberry Pi binaries') {
                    stages {
                        stage('Build Raspberry Pi binaries') {
                            steps {
                                script {
                                    buildBranch('Docker/RaspberryPi/Dockerfile', "spectreproject/spectre-raspi:${GIT_TAG_TO_USE}", "${GIT_TAG_TO_USE}", "${GIT_COMMIT_SHORT}")
                                    getChecksumfileFromImage("spectreproject/spectre-raspi:${GIT_TAG_TO_USE}", "Checksum-Spectrecoin-RaspberryPi.txt")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-RaspberryPi.txt"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Raspberry Pi image build') {
                            steps {
                                build(
                                        job: 'Spectrecoin/pi-gen/spectrecoin',
                                        parameters: [
                                                string(
                                                        name: 'SPECTRECOIN_RELEASE',
                                                        value: "${GIT_TAG_TO_USE}"
                                                ),
                                                string(
                                                        name: 'GIT_COMMIT_SHORT',
                                                        value: "${GIT_COMMIT_SHORT}"
                                                ),
                                                string(
                                                        name: 'BLOCKCHAIN_ARCHIVE_VERSION',
                                                        value: "${BLOCKCHAIN_ARCHIVE_VERSION}"
                                                )
                                        ],
                                        wait: false
                                )
                            }
                        }
                    }
                }
                stage('Build Debian binaries') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build Debian binaries') {
                            steps {
                                script {
                                    buildBranch('Docker/Debian/Dockerfile', "spectreproject/spectre-debian:${GIT_TAG_TO_USE}", "${GIT_TAG_TO_USE}", "${GIT_COMMIT_SHORT}")
                                    getChecksumfileFromImage("spectreproject/spectre-debian:${GIT_TAG_TO_USE}", "Checksum-Spectrecoin-Debian.txt")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Debian.txt"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Blockchain upload') {
                            steps {
                                build(
                                        job: 'Spectrecoin-Blockchain-v3',
                                        parameters: [
                                                string(
                                                        name: 'SPECTRECOIN_RELEASE',
                                                        value: "${GIT_TAG_TO_USE}"
                                                ),
                                                string(
                                                        name: 'SPECTRECOIN_REPOSITORY',
                                                        value: "spectre"
                                                )
                                        ],
                                        wait: false
                                )
                            }
                        }
                    }
                }
                /* CentOS build disabled, not working at the moment
                stage('Build CentOS binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch('Docker/CentOS/Dockerfile', "spectreproject/spectre-centos:${GIT_TAG_TO_USE}", "${GIT_TAG_TO_USE}", "${GIT_COMMIT_SHORT}")
                            getChecksumfileFromImage("spectreproject/spectre-centos:${GIT_TAG_TO_USE}", "Checksum-Spectrecoin-CentOS.txt")
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-CentOS.txt"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                stage('Build Fedora binaries') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch('Docker/Fedora/Dockerfile', "spectreproject/spectre-fedora:${GIT_TAG_TO_USE}", "${GIT_TAG_TO_USE}", "${GIT_COMMIT_SHORT}")
                            getChecksumfileFromImage("spectreproject/spectre-fedora:${GIT_TAG_TO_USE}", "Checksum-Spectrecoin-Fedora.txt")
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Fedora.txt"
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
                                    buildBranch('Docker/Ubuntu/Dockerfile', "spectreproject/spectre-ubuntu:${GIT_TAG_TO_USE}", "${GIT_TAG_TO_USE}", "${GIT_COMMIT_SHORT}")
                                    getChecksumfileFromImage("spectreproject/spectre-ubuntu:${GIT_TAG_TO_USE}", "Checksum-Spectrecoin-Ubuntu.txt")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Ubuntu.txt"
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
                                        job: "Spectrecoin/docker-spectrecoind/${GIT_BRANCH}",
                                        parameters: [
                                                string(
                                                        name: 'SPECTRECOIN_RELEASE',
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
                                    sh "./scripts/mac-build.sh"
                                }
                            }
                        }
                        stage('Prepare plain delivery') {
                            steps {
                                script {
                                    sh 'rm -f Spectrecoin*.dmg'
                                    prepareMacDelivery()
                                }
                            }
                        }
                        stage('Create plain delivery') {
                            steps {
                                script {
                                    sh "./scripts/mac-deployqt.sh"
                                    sh "mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                }
                            }
                        }
                        stage('Prepare OBFS4 delivery') {
                            steps {
                                script {
                                    prepareMacOBFS4Delivery()
                                }
                            }
                        }
                        stage('Create OBFS4 delivery') {
                            steps {
                                script {
                                    sh "./scripts/mac-deployqt.sh"
                                    sh "mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg"
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg"
                                }
                            }
                        }
                        stage('Upload deliveries') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "rm -f Spectrecoin*.dmg*"
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                    uploadArtifactToGitHub(
                                            user: 'spectrecoin',
                                            repository: 'spectre',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg",
                                    )
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg"
                                    uploadArtifactToGitHub(
                                            user: 'spectrecoin',
                                            repository: 'spectre',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg",
                                    )
                                    createAndArchiveChecksumFile("Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg", "Checksum-Spectrecoin-Mac.txt")
                                    createAndArchiveChecksumFile("Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg", "Checksum-Spectrecoin-Mac-OBFS4.txt")
                                    sh "rm -f Spectrecoin*.dmg* Checksum-Spectrecoin*"
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
                stage('Windows-Qt5.9.6') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
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
                                QTDIR = "${QT_DIR_WIN}"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-genbuild.bat'
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
                                    createWindowsDelivery("${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}", "-Qt5.9.6")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6.zip, Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6-OBFS4.zip"
                                }
                            }
                        }
                        stage('Upload deliveries') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "rm -f Spectrecoin*.zip*"
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6.zip"
                                    uploadArtifactToGitHub(
                                            user: 'spectrecoin',
                                            repository: 'spectre',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6.zip",
                                    )
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6-OBFS4.zip"
                                    uploadArtifactToGitHub(
                                            user: 'spectrecoin',
                                            repository: 'spectre',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6-OBFS4.zip",
                                    )
                                    createAndArchiveChecksumFile("Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6.zip", "Checksum-Spectrecoin-Win64-Qt5.9.6.txt")
                                    createAndArchiveChecksumFile("Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-Qt5.9.6-OBFS4.zip", "Checksum-Spectrecoin-Win64-Qt5.9.6-OBFS4.txt")
                                    sh "rm -f Spectrecoin*.zip* Checksum-Spectrecoin*"
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
                stage('Windows-Qt5.12.0') {
                    agent {
                        label "housekeeping"
                    }
                    stages {
                        stage('Start Windows slave') {
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
                                label "windows2"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                }
                            }
                        }
                        stage('Perform build') {
                            agent {
                                label "windows2"
                            }
                            environment {
                                QTDIR = "${QT512_DIR_WIN}"
                            }
                            steps {
                                script {
                                    bat 'scripts\\win-genbuild.bat'
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                }
                            }
                        }
                        stage('Create delivery') {
                            agent {
                                label "windows2"
                            }
                            steps {
                                script {
                                    createWindowsDelivery("${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}", "")
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip, Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip"
                                }
                            }
                        }
                        stage('Upload deliveries') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "rm -f Spectrecoin*.zip*"
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip"
                                    uploadArtifactToGitHub(
                                            user: 'spectrecoin',
                                            repository: 'spectre',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip",
                                    )
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip"
                                    uploadArtifactToGitHub(
                                            user: 'spectrecoin',
                                            repository: 'spectre',
                                            tag: "${GIT_TAG_TO_USE}",
                                            artifactNameRemote: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip",
                                    )
                                    createAndArchiveChecksumFile("Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip", "Checksum-Spectrecoin-Win64.txt")
                                    createAndArchiveChecksumFile("Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip", "Checksum-Spectrecoin-Win64-OBFS4.txt")
                                    sh "rm -f Spectrecoin*.zip* Checksum-Spectrecoin*"
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
            post {
                always {
                    script {
                        sh(
                                script:"""
                                ${WORKSPACE}/scripts/createChecksumSummary.sh \
                                    "${RELEASE_DESCRIPTION}" \
                                    "${WORKSPACE}" \
                                    "https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}"
                            """
                        )
                        editRelease(
                                user: 'spectrecoin',
                                repository: 'spectre',
                                tag: "${GIT_TAG_TO_USE}",
                                name: "${RELEASE_NAME}",
                                description: "${WORKSPACE}/releaseNotesToDeploy.txt",
                                preRelease: "${PRERELEASE}"
                        )
                        uploadArtifactToGitHub(
                                user: 'spectrecoin',
                                repository: 'spectre',
                                tag: "${GIT_TAG_TO_USE}",
                                artifactNameLocal: "releaseNotesToDeploy.txt",
                                artifactNameRemote: "RELEASENOTES.txt",
                        )
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

def buildBranch(String dockerfile, String dockerTag, String gitTag, String gitCommit) {
    withDockerRegistry(credentialsId: '051efa8c-aebd-40f7-9cfd-0053c413266e') {
        sh "docker build \\\n" +
                "-f ${dockerfile} \\\n" +
                "--rm \\\n" +
                "--build-arg GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                "--build-arg GIT_COMMIT=${gitCommit} \\\n" +
                "--build-arg SPECTRECOIN_RELEASE=${gitTag} \\\n" +
                "--build-arg REPLACE_EXISTING_ARCHIVE=--replace \\\n" +
                "-t ${dockerTag} \\\n" +
                "."
    }
}

def getChecksumfileFromImage(String dockerTag, String checksumfile) {
    withDockerRegistry(credentialsId: '051efa8c-aebd-40f7-9cfd-0053c413266e') {
        sh (
                script: """
                    docker run --name tmpContainer -dit ${dockerTag} /bin/sh 
                    docker cp tmpContainer:/filesToUpload/${checksumfile} ${checksumfile}
                    docker stop tmpContainer
                    docker rm tmpContainer
                """
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
            folderDeleteOperation(
                    folderPath: "${WORKSPACE}/src/bin/Spectrecoin.app/Contents/MacOS/Tor"),
            fileUnZipOperation(
                    filePath: "${WORKSPACE}/Tor.zip",
                    targetLocation: "${WORKSPACE}/"),
            folderDeleteOperation(
                    folderPath: "${WORKSPACE}/src/bin/debug"),
    ])
}

def prepareMacOBFS4Delivery() {
    fileOperations([
            fileRenameOperation(
                    source: "${WORKSPACE}/src/bin/Spectrecoin.app/Contents/MacOS/Tor/torrc-defaults",
                    destination: "${WORKSPACE}/src/bin/Spectrecoin.app/Contents/MacOS/Tor/torrc-defaults_plain"),
            fileRenameOperation(
                    source: "${WORKSPACE}/src/bin/Spectrecoin.app/Contents/MacOS/Tor/torrc-defaults_obfs4",
                    destination: "${WORKSPACE}/src/bin/Spectrecoin.app/Contents/MacOS/Tor/torrc-defaults"),
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

def createWindowsDelivery(String version, String suffix) {
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
    exists = fileExists "${WORKSPACE}/Spectrecoin-${version}${suffix}.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin-${version}.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin-${version}.zip"),
        ])
    }
    exists = fileExists "${WORKSPACE}/Spectrecoin-${version}-Win64${suffix}.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin-${version}-Win64${suffix}.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin-${version}-Win64${suffix}.zip"),
        ])
    }
    exists = fileExists "${WORKSPACE}/Spectrecoin-${version}-Win64${suffix}-OBFS4.zip"
    if (exists) {
        fileOperations([
                fileRenameOperation(
                        source: "${WORKSPACE}/Spectrecoin-${version}-Win64${suffix}-OBFS4.zip",
                        destination: "${WORKSPACE}/old/Spectrecoin-${version}-Win64${suffix}-OBFS4.zip"),
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
                    destination: "${WORKSPACE}/Spectrecoin-${version}-Win64${suffix}.zip"),
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
                    destination: "${WORKSPACE}/Spectrecoin-${version}-Win64${suffix}-OBFS4.zip"),
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

def createAndArchiveChecksumFile(String filename, String checksumfile) {
    sh "./scripts/createChecksums.sh $filename $checksumfile"
    archiveArtifacts allowEmptyArchive: true, artifacts: "$checksumfile"
}
