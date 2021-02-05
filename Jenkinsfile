pipeline {
  agent any
  environment {
    COMMIT_AUTHOR_EMAIL= sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim()
  }
  stages {
    stage('Build and test') {
      parallel {
        stage('Local centos7 gcc8') {
          stages {
            stage('Build') {
              steps {
                sh './jenkins/build.sh'
              }
            }
            stage('Test') {
              steps {
                script {
                  parallel (
                      'Unit tests': { sh './test/regression' },
                      'aes_nangate45': { sh './test/regression aes_nangate45' },
                      'gcd_nangate45': { sh './test/regression gcd_nangate45' },
                      'tinyRocket_nangate45': { sh './test/regression tinyRocket_nangate45' },
                      'aes_sky130hd': { sh './test/regression aes_sky130hd' },
                      'gcd_sky130hs': { sh './test/regression gcd_sky130hs' },
                      'gcd_sky130hd': { sh './test/regression gcd_sky130hd' },
                      'ibex_sky130hs': { sh './test/regression ibex_sky130hs' },
                  )
                }
              }
            }
          }
        }
        stage('Docker centos7 clang7') {
          stages{
            stage('Build') {
              steps {
                sh './jenkins/docker/build.sh centos7 clang7'
              }
            }
            stage('Test') {
              steps {
                sh './jenkins/docker/test.sh centos7 clang7'
              }
            }
          }
        }
        stage('Docker ubuntu20 gcc8') {
          stages{
            stage('Build') {
              steps {
                sh './jenkins/docker/build.sh ubuntu20 gcc8'
              }
            }
            stage('Test') {
              steps {
                sh './jenkins/docker/test.sh ubuntu20 gcc8'
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
        if ( env.BRANCH_NAME == 'master' || env.BRANCH_NAME == 'openroad' ) {
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
    always {
      sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'"
      archiveArtifacts '**/results.tgz'
    }
  }
}
