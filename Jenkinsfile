pipeline {
  agent any
  environment {
    COMMIT_AUTHOR_EMAIL= sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim()
  }
  stages {
    stage('Builds') {
      parallel {
        stage('Build local') {
          steps {
            sh './jenkins/build.sh'
          }
        }
        stage('Build docker ubuntu gcc') {
          steps {
            sh './jenkins/docker/build.sh ubuntu gcc'
          }
        }
        stage('Build docker ubuntu clang') {
          steps {
            sh './jenkins/docker/build.sh ubuntu clang'
          }
        }
        stage('Build docker centos with gcc') {
          steps {
            sh './jenkins/docker/build.sh centos gcc'
          }
        }
        stage('Build docker centos with clang') {
          steps {
            sh './jenkins/docker/build.sh centos clang'
          }
        }
      }
    }
    stage('Tests') {
      failFast true
      parallel {
        stage('Unit tests docker ubuntu gcc') {
          steps {
            sh './jenkins/docker/test.sh ubuntu gcc'
          }
        }
        stage('Unit tests docker ubuntu clang') {
          steps {
            sh './jenkins/docker/test.sh ubuntu clang'
          }
        }
        stage('Unit tests docker centos with gcc') {
          steps {
            sh './jenkins/docker/test.sh centos gcc'
          }
        }
        stage('Unit tests docker centos with clang') {
          steps {
            sh './jenkins/docker/test.sh centos clang'
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
