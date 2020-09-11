pipeline {
  agent any
  environment {
    COMMIT_AUTHOR_EMAIL= sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim()
  }
  stages {
    stage('Builds') {
      parallel {
        stage('Build centos7 gcc8') {
          steps {
            sh './jenkins/build_centos7_gcc8.sh'
          }
        }
        stage('Build docker centos7 clang7') {
          steps {
            sh './jenkins/build_docker.sh centos7 clang7'
          }
        }
        stage('Build docker ubuntu18 gcc8') {
          steps {
            sh './jenkins/build_docker.sh ubuntu18 gcc8'
          }
        }
      }
    }
    stage('Tests') {
      failFast true
      parallel {
        stage('Unit tests centos7 gcc8') {
          steps {
            sh './test/regression'
          }
        }
        stage('Unit tests docker ubuntu18 gcc8') {
          steps {
            sh './jenkins/test_docker.sh ubuntu gcc'
          }
        }
        stage('Unit tests docker centos7 clang7') {
          steps {
            sh './jenkins/test_docker.sh centos clang'
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
