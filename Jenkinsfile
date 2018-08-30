#!groovy

pipeline {
    agent any
    options {
        timestamps()
        timeout(time: 2, unit: 'HOURS')
//	ansiColor('xterm')
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '5'))
    }
    environment {
        // In case another branch beside master or develop should be deployed, enter it here
        BRANCH_TO_DEPLOY = 'xyz'
    }
    stages {
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
                            sh "cp ./Docker/CentOS/Dockerfile ."
                            docker.build("spectreproject/spectre-base", "--rm .")
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('CentOS') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/CentOS/Dockerfile ."
                            docker.build("spectreproject/spectre-base-centos", "--rm .")
                            sh "rm Dockerfile"
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Fedora/Dockerfile ."
                            docker.build("spectreproject/spectre-base-fedora", "--rm .")
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
                            docker.build("spectreproject/spectre-base-raspi", "--rm .")
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
                            sh "cp ./Docker/Ubuntu/latest/Dockerfile ."
                            docker.build("spectreproject/spectre-base-ubuntu", "--rm .")
                            sh "rm Dockerfile"
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
        stage('Build and upload Spectrecoin image') {
            when {
                anyOf { branch 'develop'; branch 'master'; branch "${BRANCH_TO_DEPLOY}" }
            }
            parallel {
                stage('Debian') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            def spectre_base = docker.build("spectreproject/spectre-base", "--rm .")
                            docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
                                spectre_base.push("latest")
                            }
                        }
                    }
                    post {
                        always {
                            sh "docker system prune --all --force"
                        }
                    }
                }
                stage('CentOS') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/CentOS/Dockerfile ."
                            def spectre_base = docker.build("spectreproject/spectre-base-centos", "--rm .")
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
                stage('Fedora') {
                    agent {
                        label "docker"
                    }
                    steps {
                        script {
                            // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
                            // So copy required Dockerfile to root dir for each build
                            sh "cp ./Docker/Fedora/Dockerfile ."
                            def spectre_base = docker.build("spectreproject/spectre-base-fedora", "--rm .")
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
                            def spectre_base = docker.build("spectreproject/spectre-base-raspi", "--rm .")
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
                            sh "cp ./Docker/Ubuntu/latest/Dockerfile ."
                            def spectre_base = docker.build("spectreproject/spectre-base-ubuntu", "--rm .")
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
            }
        }
    }
}
