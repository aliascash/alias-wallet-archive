#!groovy

node('docker') {
    def spectre_base

    stage('Clone repository') {
        checkout scm
    }

    stage('Build image') {
        // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
        // So copy required Dockerfile to root dir for each build
        sh "cp ./Docker/Debian/Dockerfile ."
        spectre_base = docker.build("spectreproject/spectre")
        sh "rm Dockerfile"
    }

    stage('Push image') {
        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
            spectre_base.push("${env.BUILD_NUMBER}")
            spectre_base.push("latest")
        }
    }
}