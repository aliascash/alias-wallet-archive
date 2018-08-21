#!groovy

node('docker') {
    def spectre_base

    stage('Clone repository') {
        checkout scm
    }

    stage('Build Debian image') {
        // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
        // So copy required Dockerfile to root dir for each build
        sh "cp ./Docker/Debian/Dockerfile ."
        spectre_base = docker.build("spectreproject/spectre")
        sh "rm Dockerfile"
    }
    stage('Push Debian image') {
        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
            spectre_base.push("${env.BUILD_NUMBER}")
            spectre_base.push("latest")
        }
    }

    stage('Build CentOS image') {
        // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
        // So copy required Dockerfile to root dir for each build
        sh "cp ./Docker/CentOS/Dockerfile ."
        spectre_base = docker.build("spectreproject/spectre-centos")
        sh "rm Dockerfile"
    }
    stage('Push CentOS image') {
        echo("Push of CentOS image disabled at the moment...")
//        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
//            spectre_base.push("${env.BUILD_NUMBER}")
//            spectre_base.push("latest")
//        }
    }

    stage('Build Fedora image') {
        // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
        // So copy required Dockerfile to root dir for each build
        sh "cp ./Docker/Fedora/Dockerfile ."
        spectre_base = docker.build("spectreproject/spectre-fedora")
        sh "rm Dockerfile"
    }
    stage('Push Fedora image') {
        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
            spectre_base.push("${env.BUILD_NUMBER}")
            spectre_base.push("latest")
        }
    }

    stage('Build Ubuntu image') {
        // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
        // So copy required Dockerfile to root dir for each build
        sh "cp ./Docker/Ubuntu/latest/Dockerfile ."
        spectre_base = docker.build("spectreproject/spectre-ubuntu")
        sh "rm Dockerfile"
    }
    stage('Push Ubuntu image') {
        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
            spectre_base.push("${env.BUILD_NUMBER}")
            spectre_base.push("latest")
        }
    }

//    stage('Build Ubuntu 16LTS image') {
//        // Copy step on Dockerfile is not working if Dockerfile is not located on root dir!
//        // So copy required Dockerfile to root dir for each build
//        sh "cp ./Docker/Ubuntu/16LTS/Dockerfile ."
//        spectre_base = docker.build("spectreproject/spectre-ubuntu-16lts")
//        sh "rm Dockerfile"
//    }
//    stage('Push Ubuntu 16LTS image') {
//        echo("Push of Ubuntu 16-LTS image disabled at the moment...")
////        docker.withRegistry('https://registry.hub.docker.com', '051efa8c-aebd-40f7-9cfd-0053c413266e') {
////            spectre_base.push("${env.BUILD_NUMBER}")
////            spectre_base.push("latest")
////        }
//    }
}