pipeline {
  agent any
  stages {
    stage('Builds') {
      parallel {
        stage('Build local') {
          steps {
            sh './jenkins/install.sh'
          }
        }
        stage('Build docker ubuntu gcc') {
          steps {
            sh './jenkins/build_docker.sh ubuntu gcc'
          }
        }
        stage('Build docker ubuntu clang') {
          steps {
            sh './jenkins/build_docker.sh ubuntu clang'
          }
        }
        stage('Build docker centos with gcc') {
          steps {
            sh './jenkins/build_docker.sh centos gcc'
          }
        }
        stage('Build docker centos with clang') {
          steps {
            sh './jenkins/build_docker.sh centos clang'
          }
        }
      }
    }
    stage('Tests') {
      failFast true
      parallel {
        stage('Unit tests docker ubuntu gcc') {
          steps {
            sh './jenkins/test_docker.sh ubuntu gcc'
          }
        }
        stage('Unit tests docker ubuntu clang') {
          steps {
            sh './jenkins/test_docker.sh ubuntu clang'
          }
        }
        stage('Unit tests docker centos with gcc') {
          steps {
            sh './jenkins/test_docker.sh centos gcc'
          }
        }
        stage('Unit tests docker centos with clang') {
          steps {
            sh './jenkins/test_docker.sh centos clang'
          }
        }
        stage('Unit tests') {
          steps {
            sh './test/regression'
          }
        }
        stage('gcd_nangate45') {
          steps {
            sh './test/regression gcd_nangate45'
          }
        }
        stage('aes_nangate45') {
          steps {
            sh './test/regression aes_nangate45'
          }
        }
        stage('tinyRocket_nangate45') {
          steps {
            sh './test/regression tinyRocket_nangate45'
          }
        }
        stage('gcd_sky130') {
          steps {
            sh './test/regression gcd_sky130'
          }
        }
        stage('aes_sky130') {
          steps {
            sh './test/regression aes_sky130'
          }
        }
        stage('ibex_sky130') {
          steps {
            sh './test/regression ibex_sky130'
          }
        }
      }
    }
  }
}
