#!groovy

pipeline {
    agent any
    options {
        timestamps()
        timeout(time: 3, unit: 'HOURS')
//	ansiColor('xterm')
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '5'))
    }
    environment {
        // In case another branch beside master or develop should be deployed, enter it here
        BRANCH_TO_DEPLOY = 'xyz'
        GITHUB_TOKEN = credentials('cdc81429-53c7-4521-81e9-83a7992bca76')
        SPECTRECOIN_VERSION='2.2.0'
        DISCORD_WEBHOOK = credentials('991ce248-5da9-4068-9aea-8a6c2c388a19')
    }
    stages {
        stage('Notification') {
            steps {
                discordSend(
                        description: "**Started build of branch $BRANCH_NAME**\n",
                        footer: 'Jenkins - the builder',
                        image: '',
                        link: "$env.BUILD_URL",
                        successful: true,
                        thumbnail: 'https://wiki.jenkins-ci.org/download/attachments/2916393/headshot.png',
                        title: "$env.JOB_NAME",
                        webhookURL: "${DISCORD_WEBHOOK}"
                )
            }
        }
        stage('Only build') {
            when {
                not {
                    anyOf { branch 'develop'; branch 'master'; branch "${BRANCH_TO_DEPLOY}" }
                }
            }
            parallel {
                stage('Debian') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            sh "cp ./Docker/Debian/Dockerfile_noUpload Dockerfile"
                            docker.build("spectreproject/spectre", "--rm .")
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
//                stage('CentOS') {
//                    agent {
//                        label "docker"
//                    }
//                    steps {
//                        script {
//                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
//                            // So copy required Dockerfile to root dir for each build
//                            sh "cp ./Docker/CentOS/Dockerfile_noUpload Dockerfile"
//                            docker.build("spectreproject/spectre-centos", "--rm .")
//                            sh "rm Dockerfile"
//                        }
//                    }
//                    post {
//                        always {
//                            sh "docker system prune --all --force"
//                        }
//                    }
//                }
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Fedora/Dockerfile_noUpload Dockerfile"
                            docker.build("spectreproject/spectre-fedora", "--rm .")
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                /* Raspi build disabled on all branches different than develop and master to increase build speed
                stage('Raspberry Pi') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/RaspberryPi/Dockerfile_noUpload Dockerfile"
                            docker.build("spectreproject/spectre-raspi", "--rm .")
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                */
                stage('Ubuntu') {
                    agent {
                        label "docker"
                    }
                    stages {
                        stage('Build Ubuntu binaries') {
                            steps {
                                script {
                                    // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                                    // So copy required Dockerfile to root dir for each build
                                    sh "cp ./Docker/Ubuntu/Dockerfile_noUpload Dockerfile"
                                    docker.build("spectreproject/spectre-ubuntu", "--rm .")
                                    sh "rm Dockerfile"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Docker image build'){
                            steps {
                                sh "echo 'Image build only triggered on develop or master builds'"
//                                build(
//                                        job: 'Spectrecoin/docker-spectrecoind/develop',
//                                        parameters: [
//                                                string(
//                                                        name: 'SPECTRECOIN_RELEASE',
//                                                        value: 'latest'
//                                                )
//                                        ]
//                                )
                            }
                        }
                    }
                }
                stage('Mac') {
                    agent {
                        label "mac"
                    }
                    steps {
                        script {
                            sh "pwd"
                            sh "df -h"
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
                                            "aws ec2 start-instances --instance-ids i-0216e564fa17a9fbd"
                                }
                            }
                        }
                        stage('Prepare build') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
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
                                    exists = fileExists "${WORKSPACE}/Spectrecoin-latest.zip"
                                    if (exists) {
                                        fileOperations([
                                                fileRenameOperation(
                                                        source: "${WORKSPACE}/Spectrecoin-latest.zip",
                                                        destination: "${WORKSPACE}/old/Spectrecoin-latest.zip"),
                                        ])
                                    }
                                    // Remove directory with artifacts from previous build
                                    // Create new delivery archive
                                    // Rename build directory back to initial name
                                    fileOperations([
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/old"),
                                            fileZipOperation("${WORKSPACE}/src/Spectrecoin"),
                                            fileRenameOperation(
                                                    source: "${WORKSPACE}/Spectrecoin.zip",
                                                    destination: "${WORKSPACE}/Spectrecoin-latest.zip"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/Spectrecoin",
                                                    destination: "${WORKSPACE}/src/bin")
                                    ])
// No upload on feature branches, only from develop and master
//                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin.zip, src/installer/Spectrecoin.msi'
//                                }
//                            }
//                        }
//                        stage('Upload delivery') {
//                            agent {
//                                label "housekeeping"
//                            }
//                            steps {
//                                script {
//                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/${BRANCH_NAME}/lastSuccessfulBuild/artifact/Spectrecoin-latest.zip"
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
                                }
                            }
                        }
                    }
                }
            }
        }
        stage('Build and upload (develop)') {
            when {
                anyOf { branch 'develop'; branch "${BRANCH_TO_DEPLOY}" }
            }
            parallel {
                stage('Debian') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Debian/Dockerfile ."
                            docker.build(
                                    "spectreproject/spectre-debian",
                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_REPOSITORY=spectre --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                            )
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
//                stage('CentOS') {
//                    agent {
//                        label "docker"
//                    }
//                    steps {
//                        script {
//                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
//                            // So copy required Dockerfile to root dir for each build
//                            sh "cp ./Docker/CentOS/Dockerfile ."
//                            docker.build(
//                                    "spectreproject/spectre-centos",
//                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_REPOSITORY=spectre --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
//                            )
//                            sh "rm Dockerfile"
//                        }
//                    }
//                    post {
//                        always {
//                            sh "docker system prune --all --force"
//                        }
//                    }
//                }
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Fedora/Dockerfile ."
                            docker.build(
                                    "spectreproject/spectre-fedora",
                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_REPOSITORY=spectre --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                            )
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Raspberry Pi') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/RaspberryPi/Dockerfile ."
                            docker.build(
                                    "spectreproject/spectre-raspi",
                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_REPOSITORY=spectre --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                            )
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Ubuntu') {
                    agent {
                        label "docker"
                    }
                    stages{
                        stage('Build Ubuntu binaries'){
                            steps {
                                script {
                                    // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                                    // So copy required Dockerfile to root dir for each build
                                    sh "cp ./Docker/Ubuntu/Dockerfile ."
                                    docker.build(
                                            "spectreproject/spectre-ubuntu",
                                            "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_REPOSITORY=spectre --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                                    )
                                    sh "rm Dockerfile"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Docker image build'){
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
                                            "aws ec2 start-instances --instance-ids i-0216e564fa17a9fbd"
                                }
                            }
                        }
                        stage('Prepare build') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
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
                                    exists = fileExists "${WORKSPACE}/Spectrecoin-latest.zip"
                                    if (exists) {
                                        fileOperations([
                                                fileRenameOperation(
                                                        source: "${WORKSPACE}/Spectrecoin-latest.zip",
                                                        destination: "${WORKSPACE}/old/Spectrecoin-latest.zip"),
                                        ])
                                    }
                                    // Remove directory with artifacts from previous build
                                    // Create new delivery archive
                                    // Rename build directory back to initial name
                                    fileOperations([
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/old"),
                                            fileZipOperation("${WORKSPACE}/src/Spectrecoin"),
                                            fileRenameOperation(
                                                    source: "${WORKSPACE}/Spectrecoin.zip",
                                                    destination: "${WORKSPACE}/Spectrecoin-latest.zip"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/Spectrecoin",
                                                    destination: "${WORKSPACE}/src/bin")
                                    ])
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-latest.zip"
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/develop/lastSuccessfulBuild/artifact/Spectrecoin-latest.zip"
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
                stage('Debian') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Debian/Dockerfile ."
                            docker.build(
                                    "spectreproject/spectre-debian",
                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_RELEASE=${SPECTRECOIN_RELEASE} --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                            )
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
//                stage('CentOS') {
//                    agent {
//                        label "docker"
//                    }
//                    steps {
//                        script {
//                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
//                            // So copy required Dockerfile to root dir for each build
//                            sh "cp ./Docker/CentOS/Dockerfile ."
//                            docker.build(
//                                    "spectreproject/spectre-centos",
//                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_RELEASE=${SPECTRECOIN_RELEASE} --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
//                            )
//                            sh "rm Dockerfile"
//                        }
//                    }
//                    post {
//                        always {
//                            sh "docker system prune --all --force"
//                        }
//                    }
//                }
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Fedora/Dockerfile ."
                            docker.build(
                                    "spectreproject/spectre-fedora",
                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_RELEASE=${SPECTRECOIN_RELEASE} --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                            )
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Raspberry Pi') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/RaspberryPi/Dockerfile ."
                            docker.build(
                                    "spectreproject/spectre-raspi",
                                    "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_RELEASE=${SPECTRECOIN_RELEASE} --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                            )
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Ubuntu') {
                    agent {
                        label "docker"
                    }
                    stages{
                        stage('Build Ubuntu binaires'){
                            steps {
                                script {
                                    // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                                    // So copy required Dockerfile to root dir for each build
                                    sh "cp ./Docker/Ubuntu/Dockerfile ."
                                    docker.build(
                                            "spectreproject/spectre-ubuntu",
                                            "--rm --build-arg GITHUB_TOKEN=${GITHUB_TOKEN} --build-arg SPECTRECOIN_RELEASE=${SPECTRECOIN_RELEASE} --build-arg REPLACE_EXISTING_ARCHIVE=--replace ."
                                    )
                                    sh "rm Dockerfile"
                                }
                            }
                            post {
                                always {
                                    sh "docker system prune --all --force"
                                }
                            }
                        }
                        stage('Trigger Docker image build'){
                            steps {
                                build(
                                        job: 'Spectrecoin/docker-spectrecoind/master',
                                        parameters: [
                                                string(
                                                        name: 'SPECTRECOIN_RELEASE',
                                                        value: "${SPECTRECOIN_RELEASE}"
                                                )
                                        ]
                                )
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
                                            "aws ec2 start-instances --instance-ids i-0216e564fa17a9fbd"
                                }
                            }
                        }
                        stage('Prepare build') {
                            agent {
                                label "windows"
                            }
                            steps {
                                script {
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
                                    exists = fileExists "${WORKSPACE}/Spectrecoin-${SPECTRECOIN_VERSION}.zip"
                                    if (exists) {
                                        fileOperations([
                                                fileRenameOperation(
                                                        source: "${WORKSPACE}/Spectrecoin-${SPECTRECOIN_VERSION}.zip",
                                                        destination: "${WORKSPACE}/old/Spectrecoin-${SPECTRECOIN_VERSION}.zip"),
                                        ])
                                    }
                                    // Remove directory with artifacts from previous build
                                    // Create new delivery archive
                                    // Rename build directory back to initial name
                                    fileOperations([
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/old"),
                                            fileZipOperation("${WORKSPACE}/src/Spectrecoin"),
                                            fileRenameOperation(
                                                    source: "${WORKSPACE}/Spectrecoin.zip",
                                                    destination: "${WORKSPACE}/Spectrecoin-${SPECTRECOIN_VERSION}.zip"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/Spectrecoin",
                                                    destination: "${WORKSPACE}/src/bin")
                                    ])
                                    archiveArtifacts allowEmptyArchive: true, artifacts: "Spectrecoin-${SPECTRECOIN_VERSION}.zip"
                                }
                            }
                        }
                        stage('Upload delivery') {
                            agent {
                                label "housekeeping"
                            }
                            steps {
                                script {
                                    sh "wget https://ci.spectreproject.io/job/Spectrecoin/job/spectre/job/master/lastSuccessfulBuild/artifact/Spectrecoin-${SPECTRECOIN_RELEASE}.zip"
                                    sh "docker run \\\n" +
                                            "--rm \\\n" +
                                            "-e GITHUB_TOKEN=${GITHUB_TOKEN} \\\n" +
                                            "-v ${WORKSPACE}:/filesToUpload \\\n" +
                                            "spectreproject/github-uploader:latest \\\n" +
                                            "github-release upload \\\n" +
                                            "    --user spectrecoin \\\n" +
                                            "    --repo spectre \\\n" +
                                            "    --tag ${SPECTRECOIN_RELEASE} \\\n" +
                                            "    --name \"Spectrecoin-${SPECTRECOIN_RELEASE}-WIN64.zip\" \\\n" +
                                            "    --file /filesToUpload/Spectrecoin-${SPECTRECOIN_RELEASE}.zip \\\n" +
                                            "    --replace"
                                    sh "rm -f Spectrecoin-${SPECTRECOIN_RELEASE}.zip"
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
                        description: "**Build:**  #$env.BUILD_NUMBER\n**Status:**  Success\n",
                        footer: 'Jenkins - the builder',
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
                    description: "**Build:**  #$env.BUILD_NUMBER\n**Status:**  Unstable\n",
                    footer: 'Jenkins - the builder',
                    image: '',
                    link: "$env.BUILD_URL",
                    successful: true,
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
                    description: "**Build:**  #$env.BUILD_NUMBER\n**Status:**  Failed\n",
                    footer: 'Jenkins - the builder',
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
