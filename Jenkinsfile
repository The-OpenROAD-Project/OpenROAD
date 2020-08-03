pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh './jenkins/build.sh'
      }
    }
    stage('Test') {
      steps {
        sh './jenkins/test.sh'
      }
    }
    stage('Build-Flow') {
      environment {
        OPENROAD_FLOW_NO_GIT_INIT = 1
      }
      steps {
        sh 'cd flow && ./build_openroad.sh'
      }
    }
  }
}
