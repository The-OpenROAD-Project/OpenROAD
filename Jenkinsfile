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
            stage('Local build') {
              steps {
                sh './jenkins/build.sh'
              }
            }
            stage('Local tests') {
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
/* disabled until osx clang build works
        stage('Docker centos clang') {
          stages{
            stage('Docker build centos clang') {
              steps {
                sh './jenkins/docker/build.sh centos clang'
              }
            }
            stage('Docker test centos clang') {
              steps {
                sh './jenkins/docker/test.sh centos clang'
              }
            }
          }
        }
        stage('Docker ubuntu clang') {
          stages{
            stage('Docker build ubuntu clang') {
              steps {
                sh './jenkins/docker/build.sh ubuntu clang'
              }
            }
            stage('Docker test ubuntu clang') {
              steps {
                sh './jenkins/docker/test.sh ubuntu clang'
              }
            }
          }
        }
*/
        stage('Docker centos gcc') {
          stages{
            stage('Docker build centos gcc') {
              steps {
                sh './jenkins/docker/build.sh centos gcc'
              }
            }
            stage('Docker test centos gcc') {
              steps {
                sh './jenkins/docker/test.sh centos gcc'
              }
            }
          }
        }
        stage('Docker ubuntu gcc') {
          stages{
            stage('Docker build ubuntu gcc') {
              steps {
                sh './jenkins/docker/build.sh ubuntu gcc'
              }
            }
            stage('Docker test ubuntu gcc') {
              steps {
                sh './jenkins/docker/test.sh ubuntu gcc'
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
