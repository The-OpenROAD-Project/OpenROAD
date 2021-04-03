pipeline {
  agent any;
  environment {
    COMMIT_AUTHOR_EMAIL = sh (returnStdout: true, script: "git --no-pager show -s --format='%ae'").trim();
    NUM_THREADS = 8
  }
  stages {
    stage('Build and test') {
      parallel {
        stage('Local centos7 gcc8') {
          stages {
            stage('Build centos7 gcc8') {
              steps {
                sh './etc/Build.sh -threads=$NUM_THREADS';
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
        stage('Local centos7 gcc8 without GUI') {
          stages {
            stage('Build centos7 gcc8 without GUI') {
              steps {
                sh './etc/Build.sh -threads=$NUM_THREADS -no-gui -dir=build-without-gui';
              }
            }
          }
        }
        stage('Docker centos7 gcc8') {
          stages{
            stage('Build centos7 gcc8') {
              steps {
                retry(3) {
                  sh 'docker pull openroad/centos7-dev ; sleep 1m'
                }
                sh './etc/DockerHelper.sh create -threads=$NUM_THREADS -os=centos7 -target=builder -compiler=gcc';
              }
            }
            stage('Test centos7 gcc8') {
              steps {
                sh './etc/DockerHelper.sh test -os=centos7 -target=builder -compiler=gcc';
              }
            }
          }
        }
        stage('Docker centos7 clang7') {
          stages{
            stage('Build centos7 clang7') {
              steps {
                retry(3) {
                  sh 'docker pull openroad/centos7-dev ; sleep 1m'
                }
                sh './etc/DockerHelper.sh create -threads=$NUM_THREADS -os=centos7 -target=builder -compiler=clang';
              }
            }
            stage('Test centos7 clang7') {
              steps {
                sh './etc/DockerHelper.sh test -os=centos7 -target=builder -compiler=clang';
              }
            }
          }
        }
        stage('Docker ubuntu20 gcc9') {
          stages{
            stage('Build ubuntu20 gcc9') {
              steps {
                retry(3) {
                  sh 'docker pull openroad/ubuntu20-dev ; sleep 1m'
                }
                sh './etc/DockerHelper.sh create -threads=$NUM_THREADS -os=ubuntu20 -target=builder -compiler=gcc';
              }
            }
            stage('Test ubuntu20 gcc9') {
              steps {
                sh './etc/DockerHelper.sh test -os=ubuntu20 -target=builder -compiler=gcc';
              }
            }
          }
        }
        stage('Docker ubuntu20 clang10') {
          stages{
            stage('Build ubuntu20 clang10') {
              steps {
                retry(3) {
                  sh 'docker pull openroad/ubuntu20-dev ; sleep 1m'
                }
                sh './etc/DockerHelper.sh create -threads=$NUM_THREADS -os=ubuntu20 -target=builder -compiler=clang';
              }
            }
            stage('Test ubuntu20 clang10') {
              steps {
                sh './etc/DockerHelper.sh test -os=ubuntu20 -target=builder -compiler=clang';
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
    always {
      sh "find . -name results -type d -exec tar zcvf {}.tgz {} ';'";
      archiveArtifacts '**/results.tgz';
    }
  }
}
