#!groovy

node {
    def spectre_base

    stage('Clone repository') {
        checkout scm
    }

    stage('Build image') {
        spectre_base = docker.build("spectreproject/spectre", "Docker/Debian")
    }

//    stage('Test image') {
//        spectre_base.inside {
//            sh 'echo "TBD: Tests"'
//        }
//    }

    stage('Push image') {
        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
            spectre_base.push("${env.BUILD_NUMBER}")
            spectre_base.push("latest")
        }
    }
}