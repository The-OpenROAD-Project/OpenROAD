pipeline {
  agent any;
  options {
    timeout(time: 1, unit: 'HOURS')
  }
  environment {
    COMMIT_AUTHOR_EMAIL = sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim();
  }
  stages {
    stage('Build and test') {
      parallel {
        stage('Local centos7 gcc8') {
          agent any;
          stages {
            stage('Build centos7 gcc8') {
              steps {
                sh './etc/Build.sh';
              }
            }
            stage('Test centos7 gcc8') {
              steps {
                script {
                  parallel (
                      'Unit tests':           { sh './test/regression' },
                      'nangate45 aes':        { sh './test/regression aes_nangate45' },
                      'nangate45 gcd':        { sh './test/regression gcd_nangate45' },
                      'nangate45 tinyRocket': { sh './test/regression tinyRocket_nangate45' },
                      'sky130hd aes':         { sh './test/regression aes_sky130hd' },
                      'sky130hd gcd':         { sh './test/regression gcd_sky130hd' },
                      'sky130hd ibex':        { sh './test/regression ibex_sky130hd' },
                      'sky130hs aes':         { sh './test/regression aes_sky130hs' },
                      'sky130hs gcd':         { sh './test/regression gcd_sky130hs' },
                      'sky130hs ibex':        { sh './test/regression ibex_sky130hs' },
                      )
                }
              }
            }
          }
          post {
            always {
              sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
              archiveArtifacts artifacts: '**/results.tgz', allowEmptyArchive: true;
            }
          }
        }
        stage('Local centos7 gcc8 without GUI') {
          agent any;
          stages {
            stage('Build centos7 gcc8 without GUI') {
              steps {
                sh './etc/Build.sh -no-gui -dir=build-without-gui';
              }
            }
          }
        }
        stage('Docker centos7 gcc8') {
          agent any;
          stages{
            stage('Pull centos7') {
              steps {
                retry(3) {
                  script {
                    try {
                      sh 'docker pull openroad/centos7-dev'
                    }
                    catch (err) {
                      echo err.getMessage();
                      sh 'sleep 1m ; exit 1';
                    }
                  }
                }
              }
            }
            stage('Build docker centos7') {
              steps {
                script {
                  parallel (
                      'build gcc8':   { sh './etc/DockerHelper.sh create -os=centos7 -target=builder -compiler=gcc' },
                      'build clang7': { sh './etc/DockerHelper.sh create -os=centos7 -target=builder -compiler=clang' },
                      )
                }
              }
            }
            stage('Test docker centos7') {
              steps {
                script {
                  parallel (
                      'test gcc8':   { sh './etc/DockerHelper.sh test -os=centos7 -target=builder -compiler=gcc' },
                      'test clang7': { sh './etc/DockerHelper.sh test -os=centos7 -target=builder -compiler=clang' },
                      )
                }
              }
            }
          }
        }
        stage('Docker ubuntu20 gcc9') {
          agent any;
          stages{
            stage('Pull ubuntu20') {
              steps {
                retry(3) {
                  script {
                    try {
                      sh 'docker pull openroad/ubuntu20-dev'
                    }
                    catch (err) {
                      echo err.getMessage();
                      sh 'sleep 1m ; exit 1';
                    }
                  }
                }
              }
            }
            stage('Build docker ubuntu20') {
              steps {
                script {
                  parallel (
                      'build gcc9':    { sh './etc/DockerHelper.sh create -os=ubuntu20 -target=builder -compiler=gcc' },
                      'build clang10': { sh './etc/DockerHelper.sh create -os=ubuntu20 -target=builder -compiler=clang' },
                      )
                }
              }
            }
            stage('Test docker ubuntu20') {
              steps {
                script {
                  parallel (
                      'test gcc9':    { sh './etc/DockerHelper.sh test -os=ubuntu20 -target=builder -compiler=gcc' },
                      'test clang10': { sh './etc/DockerHelper.sh test -os=ubuntu20 -target=builder -compiler=clang' },
                      )
                }
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
        if ( env.BRANCH_NAME == 'master' ) {
          echo('Main development branch: report to stakeholders and commit author.');
          EMAIL_TO="$COMMIT_AUTHOR_EMAIL, \$DEFAULT_RECIPIENTS, cherry@parallaxsw.com";
          REPLY_TO="$EMAIL_TO";
        } else {
          echo('Feature development branch: report only to commit author.');
          EMAIL_TO="$COMMIT_AUTHOR_EMAIL";
          REPLY_TO='$DEFAULT_REPLYTO';
        }
        emailext (
            to: "$EMAIL_TO",
            replyTo: "$REPLY_TO",
            subject: '$DEFAULT_SUBJECT',
            body: '$DEFAULT_CONTENT',
            );
      }
    }
  }
}
