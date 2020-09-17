pipeline {
  agent any
  environment {
    COMMIT_AUTHOR_EMAIL= sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim()
  }
  stages {
    stage('Build and test') {
      parallel {
        stage('Local') {
          stages {
	    stage('Build centos7 gcc8') {
              steps {
                sh './jenkins/build.sh'
              }
            }
            stage('Test centos7 gcc8') {
              steps {
                script {
                  parallel (
                      'Unit tests': { sh './test/regression' },
                      'aes_nangate45': { sh './test/regression aes_nangate45' },
                      'gcd_nangate45': { sh './test/regression gcd_nangate45' },
                      'tinyRocket_nangate45': { sh './test/regression tinyRocket_nangate45' },
                      'aes_sky130': { sh './test/regression aes_sky130' },
                      'gcd_sky130': { sh './test/regression gcd_sky130' },
                      'ibex_sky130': { sh './test/regression ibex_sky130' },
                      )
                }
              }
            }
          }
        }
        stage('Docker centos clang') {
          stages{
            stage('Build docker centos7 clang7') {
              steps {
                sh './jenkins/docker/build.sh centos7 clang7'
              }
            }
            stage('Test docker centos7 clang7') {
              steps {
                sh './jenkins/docker/test.sh centos7 clang7'
              }
            }
          }
        }
        stage('Docker centos7 gcc8') {
          stages{
            stage('Build docker centos7 gcc8') {
              steps {
                sh './jenkins/docker/build.sh centos7 gcc8'
              }
            }
            stage('Test docker centos7 gcc8') {
              steps {
                sh './jenkins/docker/test.sh centos7 gcc8'
              }
            }
          }
        }
        stage('Docker ubuntu18 gcc8') {
          stages{
            stage('Docker build ubuntu18 gcc8') {
              steps {
                sh './jenkins/docker/build.sh ubuntu18 gcc8'
              }
            }
            stage('Test docker ubuntu18 gcc7=8') {
              steps {
                sh './jenkins/docker/test.sh ubuntu18 gcc8'
              }
            }
          }
        }
      }
    }
  }
  post {
    failure {
      script {
        if ( env.BRANCH_NAME == 'openroad' ) {
          echo('Main development branch: report to stakeholders and commit author.')
          EMAIL_TO="$COMMIT_AUTHOR_EMAIL, \$DEFAULT_RECIPIENTS, cherry@parallaxsw.com"
          REPLY_TO="$EMAIL_TO"
        } else {
          echo('Feature development branch: report only to commit author.')
          EMAIL_TO="$COMMIT_AUTHOR_EMAIL"
          REPLY_TO='$DEFAULT_REPLYTO'
        }
        emailext (
            to: "$EMAIL_TO",
            replyTo: "$REPLY_TO",
            subject: '$DEFAULT_SUBJECT',
            body: '$DEFAULT_CONTENT',
            )
      }
    }
  }
}
