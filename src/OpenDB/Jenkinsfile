pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh './jenkins/build.sh'
      }
    }
    stage('Test') {
      parallel {
        stage('Python Tests') {
          steps {
            sh './jenkins/test-py.sh'
          }
        }
        stage('Unit Tests') {
          steps {
            sh './jenkins/test-unittests.sh'
          }
        }
        stage('TCL Tests') {
          steps {
            sh './jenkins/test-tcl.sh'
          }
        }
        stage('CPP Tests') {
          steps {
            sh './jenkins/test-cpp.sh'
          }
        }
      }
    }
  }
}
