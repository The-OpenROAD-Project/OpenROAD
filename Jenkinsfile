pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh './jenkins/install.sh'
      }
    }
    stage('Unit tests') {
      steps {
        sh './test/regression'
      }
    }
    stage('Flow tests') {
      failFast true
      parallel {
        stage('gcd_nangate45') {
          steps {
	    sh './test/regression gcd_nangate45'
	  }
        }
      }
    }
  }
}
