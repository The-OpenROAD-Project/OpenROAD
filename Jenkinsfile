pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh './jenkins/build.sh'
      }
    }
#    stage('Test') {
#      steps {
#        sh './jenkins/test.sh'
#      }
#    }
    stage('Build-Flow') {
      steps {
        sh label: 'Build', script: 'cd flow; ./build_openroad.sh'
      }
    }
    stage('Flow') {
      steps {
        sh './jenkins/flow.sh'
      }
    }
  }
}
