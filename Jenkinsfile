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
        SPECTRECOIN_VERSION='2.1.1'
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
        stage('Build Spectrecoin image') {
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
                            sh "cp ./Docker/Debian/Dockerfile ."
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
//                            sh "cp ./Docker/CentOS/Dockerfile ."
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
                            sh "cp ./Docker/Fedora/Dockerfile ."
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
                            sh "cp ./Docker/RaspberryPi/Dockerfile ."
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
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Ubuntu/Dockerfile ."
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
                        stage('Build Windows wallet') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "C:\\Qt\\5.9.6\\msvc2017_64"
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
                                    bat 'scripts\\win-build.bat'
                                    fileOperations([
                                            fileUnZipOperation(
                                                    filePath: "${WORKSPACE}/Tor.zip",
                                                    targetLocation: "${WORKSPACE}/"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/bin/debug"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/Spectrecoin"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/bin",
                                                    destination: "${WORKSPACE}/src/Spectrecoin"),
                                            fileZipOperation("${WORKSPACE}/src/Spectrecoin"),
                                            fileRenameOperation(
                                                    source: "${WORKSPACE}/Spectrecoin.zip",
                                                    destination: "${WORKSPACE}/Spectrecoin-latest.zip"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/Spectrecoin",
                                                    destination: "${WORKSPACE}/src/bin")
                                    ])
//                                    bat 'scripts\\win-installer.bat'
//                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin.zip, src/installer/Spectrecoin.msi'
                                }
                            }
                        }
                    }
                }
            }
        }
        stage('Build and upload Spectrecoin image (develop)') {
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
                            def spectre_base = docker.build("spectreproject/spectre", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_base.push("latest")
                            }
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
//                            def spectre_base = docker.build("spectreproject/spectre-centos", "--rm .")
//                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
//                                spectre_base.push("latest")
//                            }
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
                            def spectre_base = docker.build("spectreproject/spectre-fedora", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_base.push("latest")
                            }
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
                            def spectre_base = docker.build("spectreproject/spectre-raspi", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_base.push("latest")
                            }
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
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Ubuntu/Dockerfile ."
                            def spectre_base = docker.build("spectreproject/spectre-ubuntu", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_base.push("latest")
                            }
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
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
                        stage('Build Windows wallet') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "C:\\Qt\\5.9.6\\msvc2017_64"
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
                                    bat 'scripts\\win-build.bat'
                                    fileOperations([
                                            fileUnZipOperation(
                                                    filePath: "${WORKSPACE}/Tor.zip",
                                                    targetLocation: "${WORKSPACE}/"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/bin/debug"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/Spectrecoin"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/bin",
                                                    destination: "${WORKSPACE}/src/Spectrecoin"),
                                            fileZipOperation("${WORKSPACE}/src/Spectrecoin"),
                                            fileRenameOperation(
                                                    source: "${WORKSPACE}/Spectrecoin.zip",
                                                    destination: "${WORKSPACE}/Spectrecoin-latest.zip"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/Spectrecoin",
                                                    destination: "${WORKSPACE}/src/bin")
                                    ])
//                                    bat 'scripts\\win-installer.bat'
                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin-latest.zip, src/installer/Spectrecoin.msi'
                                }
                            }
                        }
                    }
                }
            }
            post {
                success {
                    build job: 'Spectrecoin/spectre-distribution/develop', wait: false
                }
            }
        }
        stage('Build and upload Spectrecoin image (release)') {
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
                            def spectre_image = docker.build("spectreproject/spectre", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_image.push("${SPECTRECOIN_VERSION}")
                            }
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
//                            def spectre_image = docker.build("spectreproject/spectre-centos", "--rm .")
//                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
//                                spectre_image.push("${SPECTRECOIN_VERSION}")
//                            }
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
                            def spectre_image = docker.build("spectreproject/spectre-fedora", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_image.push("${SPECTRECOIN_VERSION}")
                            }
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
                            def spectre_image = docker.build("spectreproject/spectre-raspi", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_image.push("${SPECTRECOIN_VERSION}")
                            }
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
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Ubuntu/Dockerfile ."
                            def spectre_image = docker.build("spectreproject/spectre-ubuntu", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_image.push("${SPECTRECOIN_VERSION}")
                            }
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
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
                        stage('Build Windows wallet') {
                            agent {
                                label "windows"
                            }
                            environment {
                                QTDIR = "C:\\Qt\\5.9.6\\msvc2017_64"
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
                                    bat 'scripts\\win-build.bat'
                                    fileOperations([
                                            fileUnZipOperation(
                                                    filePath: "${WORKSPACE}/Tor.zip",
                                                    targetLocation: "${WORKSPACE}/"),
                                            folderDeleteOperation(
                                                    folderPath: "${WORKSPACE}/src/bin/debug"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/bin",
                                                    destination: "${WORKSPACE}/src/Spectrecoin"),
                                            fileZipOperation("${WORKSPACE}/src/Spectrecoin"),
                                            fileRenameOperation(
                                                    source: "${WORKSPACE}/Spectrecoin.zip",
                                                    destination: "${WORKSPACE}/Spectrecoin-${SPECTRECOIN_VERSION}.zip"),
                                            folderRenameOperation(
                                                    source: "${WORKSPACE}/src/Spectrecoin",
                                                    destination: "${WORKSPACE}/src/bin")
                                    ])
//                                    bat 'scripts\\win-installer.bat'
                                    archiveArtifacts allowEmptyArchive: true, artifacts: 'Spectrecoin-${SPECTRECOIN_VERSION}.zip, src/installer/Spectrecoin.msi'
                                }
                            }
                        }
                    }
                }
            }
            post {
                success {
                    build job: 'Spectrecoin/spectre-distribution/master', wait: false
                }
            }
        }
    }
    post {
        success {
            script {
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
