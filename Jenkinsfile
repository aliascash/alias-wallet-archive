#!groovy

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
        BRANCH_TO_DEPLOY = 'xyz'
        DISCORD_WEBHOOK = credentials('991ce248-5da9-4068-9aea-8a6c2c388a19')
        GITHUB_TOKEN = credentials('cdc81429-53c7-4521-81e9-83a7992bca76')
        DEVELOP_TAG = "Build${BUILD_NUMBER}"
        RELEASE_TAG = sh(
                script: "printf \$(grep CLIENT_VERSION_MAJOR src/clientversion.h | tr -s ' ' | cut -d ' ' -f3 | tr -d '\\n' | tr -d '\\r').\$(grep CLIENT_VERSION_MINOR src/clientversion.h | tr -s ' ' | cut -d ' ' -f3 | tr -d '\\n' | tr -d '\\r').\$(grep CLIENT_VERSION_REVISION src/clientversion.h | tr -s ' ' | cut -d ' ' -f3 | tr -d '\\n' | tr -d '\\r')",
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
                /*
                stage('Debian Stretch') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Debian/Dockerfile_Stretch_noUpload',
                                    dockerTag: "spectreproject/spectre-debian-stretch:${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                stage('Debian Buster') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Debian/Dockerfile_Buster_noUpload',
                                    dockerTag: "spectreproject/spectre-debian-buster:${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                /* Raspi build disabled on all branches different than develop and master to increase build speed
                stage('Raspberry Pi Buster') {
                    agent {
                        label "raspi-builder"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/RaspberryPi/Dockerfile_Buster_noUpload',
                                    dockerTag: "spectreproject/spectre-raspi-buster:${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Fedora/Dockerfile_noUpload',
                                    dockerTag: "spectreproject/spectre-fedora:${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
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
                                    dockerTag: "spectreproject/spectre-ubuntu-18-04:${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Ubuntu 19.04') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Ubuntu/Dockerfile_19_04_noUpload',
                                    dockerTag: "spectreproject/spectre-ubuntu-19-04:${GIT_TAG_TO_USE}"
                            )
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Ubuntu 19.10') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildFeatureBranch(
                                    dockerfile: 'Docker/Ubuntu/Dockerfile_19_10_noUpload',
                                    dockerTag: "spectreproject/spectre-ubuntu-19-10:${GIT_TAG_TO_USE}"
                            )
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
                        BOOST_PATH = "${BOOST_PATH_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        QT_PATH = "${QT_PATH_MAC}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.10
                    }
                    steps {
                        script {
                            sh(
                                    script: """
                                        pwd
                                        ./scripts/mac-build.sh
                                        rm -f Spectrecoin*.dmg
                                    """
                            )
                            prepareMacDelivery()
                            sh(
                                    script: """
                                        ./scripts/mac-deployqt.sh
                                        mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg
                                    """
                            )
                            // Archive step here only to be able to make feature branch builds available for download
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                            prepareMacOBFS4Delivery()
                            sh(
                                    script: """
                                        ./scripts/mac-deployqt.sh
                                        mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg
                                    """
                            )
                        }
                    }
                }
                /*
                stage('Windows-Qt5.9.6') {
                    stages {
                        stage('Start Windows slave') {
                            steps {
                                withCredentials([[
                                                         $class           : 'AmazonWebServicesCredentialsBinding',
                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
                                                 ]]) {
                                    sh(
                                            script: """
                                                docker run \
                                                    --rm \
                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
                                                    garland/aws-cli-docker \
                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
                                            """
                                    )
                                }
                            }
                        }
                        stage('Windows build') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "${QT_DIR_WIN}"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                    bat 'scripts\\win-genbuild.bat'
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                    createWindowsDelivery(
                                            version: "${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}",
                                            suffix: "-Qt5.9.6"
                                    )
                                }
                            }
                        }
                    }
                }
                */
                stage('Windows-Qt5.12.x') {
                    stages {
                        stage('Start Windows slave') {
                            steps {
                                withCredentials([[
                                                         $class           : 'AmazonWebServicesCredentialsBinding',
                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
                                                 ]]) {
                                    sh(
                                            script: """
                                                docker run \
                                                    --rm \
                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
                                                    garland/aws-cli-docker \
                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
                                            """
                                    )
                                }
                            }
                        }
                        stage('Windows build') {
                            agent {
                                label "windows2"
                            }
                            environment {
                                QTDIR = "${QT512_DIR_WIN}"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                    bat 'scripts\\win-genbuild.bat'
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                    createWindowsDelivery(
                                            version: "${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}",
                                            suffix: ""
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip"
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
        stage('Develop/Master') {
            when {
                anyOf { branch 'master'; branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            //noinspection GroovyAssignabilityCheck
            parallel {
                stage('Raspberry Pi Buster') {
                    agent {
                        label "raspi-builder"
                    }
                    stages {
                        stage('Binary build') {
                            steps {
                                script {
                                    buildBranch(
                                            dockerfile: 'Docker/RaspberryPi/Dockerfile_Buster',
                                            dockerTag: "spectreproject/spectre-raspi-buster:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "spectreproject/spectre-raspi-buster:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Spectrecoin-RaspberryPi-Buster.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-RaspberryPi-Buster.txt"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger image build') {
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
                                                )
                                        ],
                                        wait: false
                                )
                            }
                        }
                    }
                }
                /*
                stage('Debian Stretch') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build binaries') {
                            steps {
                                script {
                                    buildBranch(
                                            dockerfile: 'Docker/Debian/Dockerfile_Stretch',
                                            dockerTag: "spectreproject/spectre-debian-stretch:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "spectreproject/spectre-debian-stretch:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Spectrecoin-Debian-Stretch.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Debian-Stretch.txt"
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
                */
                stage('Debian Buster') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build binaries') {
                            steps {
                                script {
                                    buildBranch(
                                            dockerfile: 'Docker/Debian/Dockerfile_Buster',
                                            dockerTag: "spectreproject/spectre-debian-buster:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "spectreproject/spectre-debian-buster:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Spectrecoin-Debian-Buster.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Debian-Buster.txt"
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
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            buildBranch(
                                    dockerfile: 'Docker/Fedora/Dockerfile',
                                    dockerTag: "spectreproject/spectre-fedora:${GIT_TAG_TO_USE}",
                                    gitTag: "${GIT_TAG_TO_USE}",
                                    gitCommit: "${GIT_COMMIT_SHORT}"
                            )
                            getChecksumfileFromImage(
                                    dockerTag: "spectreproject/spectre-fedora:${GIT_TAG_TO_USE}",
                                    checksumfile: "Checksum-Spectrecoin-Fedora.txt"
                            )
                            archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Fedora.txt"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Ubuntu 18.04') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build binaries') {
                            steps {
                                script {
                                    buildBranch(
                                            dockerfile: 'Docker/Ubuntu/Dockerfile_18_04',
                                            dockerTag: "spectreproject/spectre-ubuntu-18-04:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "spectreproject/spectre-ubuntu-18-04:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Spectrecoin-Ubuntu-18-04.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Ubuntu-18-04.txt"
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
                stage('Ubuntu-19-04') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build binaries') {
                            steps {
                                script {
                                    buildBranch(
                                            dockerfile: 'Docker/Ubuntu/Dockerfile_19_04',
                                            dockerTag: "spectreproject/spectre-ubuntu-19-04:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "spectreproject/spectre-ubuntu-19-04:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Spectrecoin-Ubuntu-19-04.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Ubuntu-19-04.txt"
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
                stage('Ubuntu-19-10') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build binaries') {
                            steps {
                                script {
                                    buildBranch(
                                            dockerfile: 'Docker/Ubuntu/Dockerfile_19_10',
                                            dockerTag: "spectreproject/spectre-ubuntu-19-10:${GIT_TAG_TO_USE}",
                                            gitTag: "${GIT_TAG_TO_USE}",
                                            gitCommit: "${GIT_COMMIT_SHORT}"
                                    )
                                    getChecksumfileFromImage(
                                            dockerTag: "spectreproject/spectre-ubuntu-19-10:${GIT_TAG_TO_USE}",
                                            checksumfile: "Checksum-Spectrecoin-Ubuntu-19-10.txt"
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Checksum-Spectrecoin-Ubuntu-19-10.txt"
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
                stage('Mac') {
                    agent {
                        label "mac"
                    }
                    environment {
                        BOOST_PATH = "${BOOST_PATH_MAC}"
                        OPENSSL_PATH = "${OPENSSL_PATH_MAC}"
                        QT_PATH = "${QT_PATH_MAC}"
                        PATH = "/usr/local/bin:${QT_PATH}/bin:$PATH"
                        MACOSX_DEPLOYMENT_TARGET = 10.10
                    }
                    stages {
                        stage('MacOS build') {
                            steps {
                                script {
                                    sh(
                                            script: """
                                                pwd
                                                ./scripts/mac-build.sh
                                                rm -f Spectrecoin*.dmg
                                            """
                                    )
                                    prepareMacDelivery()
                                    sh(
                                            script: """
                                                ./scripts/mac-deployqt.sh
                                                mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg
                                            """
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg"
                                    prepareMacOBFS4Delivery()
                                    sh(
                                            script: """
                                                ./scripts/mac-deployqt.sh
                                                mv Spectrecoin.dmg Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg
                                            """
                                    )
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
                                    sh(
                                            script: """
                                                rm -f Spectrecoin*.dmg*
                                                wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg
                                            """
                                    )
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
                                    createAndArchiveChecksumFile(
                                            filename: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac.dmg",
                                            checksumfile: "Checksum-Spectrecoin-Mac.txt"
                                    )
                                    createAndArchiveChecksumFile(
                                            filename: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Mac-OBFS4.dmg",
                                            checksumfile: "Checksum-Spectrecoin-Mac-OBFS4.txt"
                                    )
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
                stage('Windows-Qt5.12.x') {
                    stages {
                        stage('Start Windows slave') {
                            steps {
                                withCredentials([[
                                                         $class           : 'AmazonWebServicesCredentialsBinding',
                                                         credentialsId    : '91c4a308-07cd-4468-896c-3d75d086190d',
                                                         accessKeyVariable: 'AWS_ACCESS_KEY_ID',
                                                         secretKeyVariable: 'AWS_SECRET_ACCESS_KEY'
                                                 ]]) {
                                    sh(
                                            script: """
                                                docker run \
                                                    --rm \
                                                    --env AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
                                                    --env AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
                                                    --env AWS_DEFAULT_REGION=eu-west-1 \
                                                    garland/aws-cli-docker \
                                                    aws ec2 start-instances --instance-ids i-06fb7942772e77e55
                                            """
                                    )
                                }
                            }
                        }
                        stage('Windows build') {
                            agent {
                                label "windows2"
                            }
                            environment {
                                QTDIR = "${QT512_DIR_WIN}"
                            }
                            steps {
                                script {
                                    prepareWindowsBuild()
                                    bat 'scripts\\win-genbuild.bat'
                                    bat 'scripts\\win-build.bat'
//                                    bat 'scripts\\win-installer.bat'
                                    createWindowsDelivery(
                                            version: "${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}",
                                            suffix: ""
                                    )
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip, Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip"
                                }
                            }
                        }
                        stage('Upload deliveries') {
                            steps {
                                script {
                                    sh(
                                            script: """
                                                rm -f Spectrecoin-*-Win64.zip Spectrecoin-*-Win64-OBFS4.zip
                                                wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${GIT_BRANCH}/${BUILD_NUMBER}/artifact/Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip
                                            """
                                    )
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
                                    createAndArchiveChecksumFile(
                                            filename: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64.zip",
                                            checksumfile: "Checksum-Spectrecoin-Win64.txt"
                                    )
                                    createAndArchiveChecksumFile(
                                            filename: "Spectrecoin-${GIT_TAG_TO_USE}-${GIT_COMMIT_SHORT}-Win64-OBFS4.zip",
                                            checksumfile: "Checksum-Spectrecoin-Win64-OBFS4.txt"
                                    )
                                    sh "rm -f Spectrecoin*.zip* Checksum-Spectrecoin*"
                                }
                            }
                            /*
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                            */
                        }
                    }
                }
            }
            post {
                always {
                    script {
                        sh(
                            script: """
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
                        sh "docker system prune --all --force"
                    }
                }
            }
        }
        stage('Update download links') {
            when {
                branch 'master'
            }
            steps {
                build(
                        job: 'updateDownloadURLs',
                        parameters: [
                                string(
                                        name: 'RELEASE_VERSION',
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
